# Dual-function PowerShell du
# Licensed under the MIT license.
# Based on multiple answers at <https://stackoverflow.com/questions/868264/du-in-powershell>.
function Get-ChildInfo([System.String] $Path = ".", [Switch] $Recurse) {
    # Format file sizes in bytes into human-readable form
    function Format-FileSize([Int64] $nbytes) {
        if ($nbytes -lt 1024) {
            return "{0:0.0} B" -f $nbytes
        }
        if ($nbytes -lt 1MB) {
            return "{0:0.0} KiB" -f ($nbytes / 1KB)
        }
        if ($nbytes -lt 1GB) {
            return "{0:0.0} MiB" -f ($nbytes / 1MB)
        }
        return "{0:0.0} GiB" -f ($nbytes / 1GB)
    }

    # Reverse the pipeline
    function Get-Reversed {
        $arr = @($input)
        [Array]::reverse($arr)
        $arr
    }

    if ($Recurse) {
        # Recursive, equivelant to unix's `du -h`.
        # Recursively get all sub directories and their sized without their sub directories
        $subdirs = Get-ChildItem -Recurse -File $Path |
        Group-Object DirectoryName |
        Select-Object Name, @{Name = "Length"; Expression = { ($_.Group | Measure-Object -Sum Length).Sum } }
        # Sum up each sub directory's sizes
        $subdirs |
        Select-Object Name, @{
            Name       = "Size"
            Expression = {
                $thisDir = $_
                $mySubs = $subdirs | Where-Object { $_.Name.StartsWith($thisDir.Name) }
                Format-FileSize(($mySubs | Measure-Object -Sum Length).Sum)
            }
        } |
        # Reverse the order to mimic unix du
        Get-Reversed
    }
    else {
        # Summative, equivalent to unix's `du -hd0 * | sort -h`.
        Get-ChildItem $Path |
        ForEach-Object {
            $f = $_
            # Calculate the sum size of this sub directory
            $result = Get-ChildItem -r $f.FullName |
            Measure-Object -Sum Length |
            Select-Object @{Name = "Name"; Expression = { $f.Name } }, Sum
            # Still yield a result if the sub directory is empty
            if ($null -eq $result) {
                New-Object PSObject -Property @{Name = $f.Name; Sum = 0 }
            }
            else {
                $result
            }
        } |
        # Sort entries from large to small
        Sort-Object -Descending Sum |
        # Filter the nbytes into human-readable form
        Select-Object Name, @{Name = "Size"; Expression = { Format-FileSize($_.Sum) } }
    }
}

# Alias Get-ChildInfo to du
Set-Alias du Get-ChildInfo
