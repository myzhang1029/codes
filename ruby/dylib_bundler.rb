# A spinoff project from https://github.com/myzhang1029/homebrew-myzhang1029/blob/main/Formula/scopy.rb
# Pure-Ruby replacement for https://github.com/auriamg/macdylibbundler that is completely non-interactive

require "pathname"
require "macho"


DEFAULT_WANTED_INSTALL_NAME_BASE = "@executable_path/../Frameworks".freeze

# Instead of using dylibbundler, we use our own script to copy and dereference
# all the dylibs.
def copy_framework(libpath, targetdir, referring_macho, old_name, rpath_search, wanted_install_name_base)
  # Copy a framework; see `copy_macho_dep` for argument description.
  #
  # Find the first parent directory that ends with .framework
  frameworkpath = libpath
  frameworkpath = frameworkpath.parent until frameworkpath.basename.to_s.end_with?(".framework")
  # Now the basename would be the framework name, like `iio.framework`
  frameworkname = frameworkpath.basename
  # Copy the framework to the destination if it doesn't exist
  dest = targetdir + frameworkname
  cp_r frameworkpath, targetdir unless dest.exist?
  # Find the relative path of the macho, from the framework bundle root,
  # like `Versions/0.26/iio`.
  # Prepend the framework name to form the new install name
  relative_path = frameworkname + libpath.relative_path_from(frameworkpath)
  new_name = "#{wanted_install_name_base}/#{relative_path}"
  referring_macho.change_install_name(old_name, new_name)
  fix_dylib(targetdir + relative_path, targetdir, rpath_search, Pathname.new(referring_macho.filename),
            wanted_install_name_base)
end

def copy_single_dylib(libpath, targetdir, referring_macho, old_name, rpath_search, wanted_install_name_base)
  # Copy a single .dylib; see `copy_macho_dep` for argument description.
  basename = libpath.basename
  dest = targetdir + basename
  cp libpath, targetdir unless dest.exist?
  new_name = "#{wanted_install_name_base}/#{basename}"
  referring_macho.change_install_name(old_name, new_name)
  fix_dylib(dest, targetdir, rpath_search, Pathname.new(referring_macho.filename), wanted_install_name_base)
end

def copy_macho_dep(libpath, targetdir, referring_macho, old_name, rpath_search, wanted_install_name_base)
  # Copy `libpath` to `targetdir`.
  #
  # - `libpath` should be the on-disk location of `old_name`. They may be the same.
  #   If `libpath` refers to a framework, it should point to the Mach-O file inside the bundle,
  #   like `some/path/iio.framework/Versions/0.26/iio`, instead of the bundle root.
  #
  # - `targetdir` is the physical location of the `Frameworks` directory of the application bundle.
  #
  # - `referring_macho` is the original file that links to `libpath`.
  #   Its loader table (the `LC_LOAD_DYLIB` command) will be updated to point to
  #   `wanted_install_name_base`, and its path will be used to compute `@loader_path`.
  #
  # - `old_name` should be an entry in the loader table of `referring_macho`.
  #
  # - `rpath_search` is an array of `Pathname`s where we try to locate `@rpath` entries.
  #   For this function, `rpath_search` is only used to recursively call `fix_dylib`.
  #
  # - `wanted_install_name_base` is a relative reference to `targetdir`.
  #   A sane default is `DEFAULT_WANTED_INSTALL_NAME_BASE`.
  if libpath.to_s.include?(".framework")
    copy_framework(libpath, targetdir, referring_macho, old_name, rpath_search, wanted_install_name_base)
  else
    copy_single_dylib(libpath, targetdir, referring_macho, old_name, rpath_search, wanted_install_name_base)
  end
end

def fix_dylib(machopath, targetdir, rpath_search, loader_machopath = nil,
              wanted_install_name_base = DEFAULT_WANTED_INSTALL_NAME_BASE)
  # Copy non-system dylibs referred by `machopath` to `targetdir`, and update
  # the install names to point to the new destination via a relative path based
  # from `wanted_install_name_base`.
  #
  # The caller should ensure that `wanted_install_name_base` resolves to
  # `targetdir` in this Mach-O.
  chmod "u+w", machopath
  macho = MachO.open(machopath)

  # Precompute `loader_path` in case used
  # Find the dirname from the dirname of `macho`'s loader (containing the `.dylib` or the `.Framework`)
  if loader_machopath.nil?
    loader_path = nil
  elsif loader_machopath.to_s.include?(".framework")
    loader_path = loader_machopath
    loader_path = loader_path.parent until loader_path.basename.to_s.end_with?(".framework")
    loader_path = loader_path.parent
  else
    loader_path = loader_machopath.parent
  end

  macho.linked_dylibs.each do |libpathstr|
    # Do not copy macOS system libraries
    next if libpathstr.start_with?("/usr/lib", "/System/Library")

    if libpathstr.start_with?("/")
      # Simple logic for absolute path references (we already skipped system libraries)
      libpath = Pathname.new(libpathstr)
      copy_macho_dep(libpath, targetdir, macho, libpathstr, rpath_search, wanted_install_name_base)
    elsif libpathstr.start_with?("@rpath") || !libpathstr.start_with?("@")
      # A @rpath/.., or a relative path. We deal with them on a best-effort basis.
      lib_fromrpath = libpathstr.delete_prefix("@rpath/")

      if (targetdir + lib_fromrpath).exist?
        # This framework or dylib is already in `targetdir`
        # We just change the install name
        macho.change_install_name(libpathstr, "#{wanted_install_name_base}/#{lib_fromrpath}")
        next
      end

      found_parent = rpath_search.find do |candidate|
        (candidate + lib_fromrpath).exist?
      end
      if found_parent
        copy_macho_dep(found_parent + lib_fromrpath, targetdir, macho, libpathstr, rpath_search,
                       wanted_install_name_base)
      else
        opoo "Warning: Cannot resolve rpath #{libpathstr}"
      end
    elsif libpathstr.start_with?("@loader_path")
      if loader_path.nil?
        opoo "Warning: Not expecting a @loader_path in Mach-O #{machopath}"
        next
      end
      lib_fromlpath = libpathstr.delete_prefix("@loader_path/")
      if (targetdir + lib_fromlpath).exist?
        macho.change_install_name(libpathstr, "#{wanted_install_name_base}/#{lib_fromlpath}")
      elsif (loader_path + lib_fromlpath).exist?
        copy_macho_dep(loader_path + lib_fromlpath, targetdir, macho, libpathstr, rpath_search,
                       wanted_install_name_base)
      else
        opoo "Warning: Cannot resolve @loader_path #{libpathstr}"
      end
      # else: a @... path that we don't know how to handle. Leave it as-is
    end
  end
  # Libraries added by the Scopy build script uses @rpath; make sure they still work
  if macho.rpaths.exclude?(wanted_install_name_base) && macho.rpaths.exclude?(wanted_install_name_base + "/")
    macho.add_rpath(wanted_install_name_base)
  end
  # Don't forget to write the changes
  macho.write!
end
