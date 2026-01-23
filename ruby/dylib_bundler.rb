# A spinoff project from https://github.com/myzhang1029/homebrew-myzhang1029/blob/main/Formula/scopy.rb
# Pure-Ruby replacement for https://github.com/auriamg/macdylibbundler that is completely non-interactive

require "pathname"
require "macho"

WANTED_RPATH_BASE = "@executable_path/../Frameworks".freeze

# Instead of using dylibbundler, we use our own script to copy and dereference
# all the dylibs.
def copy_framework(libpath, targetdir, referring_macho, old_name, rpath_search)
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
  new_name = "#{WANTED_RPATH_BASE}/#{relative_path}"
  referring_macho.change_install_name(old_name, new_name)
  fix_dylib(targetdir + relative_path, targetdir, rpath_search)
end

def copy_single_dylib(libpath, targetdir, referring_macho, old_name, rpath_search)
  # Copy a single .dylib; see `copy_macho_dep` for argument description.
  basename = libpath.basename
  dest = targetdir + basename
  cp libpath, targetdir unless dest.exist?
  new_name = "#{WANTED_RPATH_BASE}/#{basename}"
  referring_macho.change_install_name(old_name, new_name)
  fix_dylib(dest, targetdir, rpath_search)
end

def copy_macho_dep(libpath, targetdir, referring_macho, old_name, rpath_search)
  # Copy `libpath` to `targetdir`.
  #
  # - `old_name` should be an entry in the loader table of `referring_macho`.
  #
  # - `libpath` should be the on-disk location of `old_name`. They may be the same.
  #   If `libpath` refers to a framework, it should point to the Mach-O file inside the bundle,
  #   like `some/path/iio.framework/Versions/0.26/iio`, instead of the bundle root.
  #
  # - `targetdir` is the physical location of the `Frameworks` directory of the application bundle.
  #
  # - `referring_macho` is the original file that links to `libpath`.
  #   Its loader table (the `LC_LOAD_DYLIB` command) will be updated to point to `WANTED_RPATH_BASE`.
  #
  # - `rpath_search` is only used to recursively call `fix_dylib`.
  if libpath.to_s.include?(".framework")
    copy_framework(libpath, targetdir, referring_macho, old_name, rpath_search)
  else
    copy_single_dylib(libpath, targetdir, referring_macho, old_name, rpath_search)
  end
end

def fix_dylib(machopath, targetdir, rpath_search)
  # Copy non-system dylibs referred by `machopath` to `targetdir`, and update
  # the install names to point to the new destination via an `@executable_path` relative path.
  #
  # `rpath_search` is an array of `Pathname`s where we try to locate `@rpath` entries.
  chmod "u+w", machopath
  macho = MachO.open(machopath)

  macho.linked_dylibs.each do |libpathstr|
    # Do not copy macOS system libraries
    next if libpathstr.start_with?("/usr/lib", "/System/Library")

    if libpathstr.start_with?("/")
      # Simple logic for absolute path references (we already skipped system libraries)
      libpath = Pathname.new(libpathstr)
      copy_macho_dep(libpath, targetdir, macho, libpathstr, rpath_search)
    elsif libpathstr.start_with?("@rpath") || !libpathstr.start_with?("@")
      # A @rpath/.., or a relative path. We deal with them on a best-effort basis.
      lib_fromrpath = libpathstr.delete_prefix("@rpath/")

      if (targetdir + lib_fromrpath).exist?
        # This framework or dylib is already in `targetdir`
        # We just change the install name
        macho.change_install_name(libpathstr, "#{WANTED_RPATH_BASE}/#{lib_fromrpath}")
        next
      end

      found_parent = rpath_search.find do |candidate|
        (candidate + lib_fromrpath).exist?
      end
      if found_parent
        copy_macho_dep(found_parent + lib_fromrpath, targetdir, macho, libpathstr, rpath_search)
      else
        opoo "Warning: Cannot resolve rpath #{libpathstr}"
      end
      # else: a @... path that we don't know how to handle. Leave it as-is
    end
  end
  # Libraries added by the Scopy build script uses @rpath; make sure they still work
  if macho.rpaths.exclude?(WANTED_RPATH_BASE) && macho.rpaths.exclude?(WANTED_RPATH_BASE + "/")
    macho.add_rpath(WANTED_RPATH_BASE)
  end
  # Don't forget to write the changes
  macho.write!
end
