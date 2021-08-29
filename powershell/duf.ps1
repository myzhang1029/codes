# Dual-function PowerShell du
# Licensed under the MIT license.
# Based on multiple answers at <https://stackoverflow.com/questions/868264/du-in-powershell>.
function du_full(
        [System.String]
        $Path=“.”,
        [switch]
        $Recurse
    ) {
    # Format file sizes in bytes into human-readable form
    function Format-FileSize([int64] $nbytes) {
        if ($nbytes -lt 1024)
        {
            return $nbytes
        }
        if ($nbytes -lt 1MB)
        {
            return "{0:0.0} KiB” -f ($nbytes/1KB)
        }
        if ($nbytes -lt 1GB)
        {
            return "{0:0.0} MiB” -f ($nbytes/1MB)
        }
        return "{0:0.0} GiB“ -f ($nbytes/1GB)
    }

    if ($Recurse) {
        # Recursive, equivelant to unix's `du -h`.
        # TODO BUG
        Get-ChildItem -File $Path -Recurse |
            Group-Object directoryName |
            Select name, @{Name=‘Length'; Expression={($_.group | Measure-Object -sum length).sum }} |
            % {
                $dn = $_
                $size = ($groupedList | where { $_.Name -Like "$($dn.Name)*" } | Measure-Object -Sum Length).Sum
                New-Object PSObject -Property @{
                    Name=Resolve-Path -Relative $dn.Name
                    Size=Format-FileSize($size)
                }
            }
    } else {
        # Summative, equivalent to unix's `du -hd0`.
        # TODO BUG: Now it ignores empty sub-directories
        Get-ChildItem $Path | 
            % {
                $f = $_
                # Calculate the sum size of this sub directory
                Get-ChildItem -r $_.FullName |
                    Measure-Object -Sum Length |
                    Select @{Name="Name";Expression={$f}}, Sum
            } |
            Sort-Object -Descending Sum |
            # Filter the nbytes into human-readable form
            Select Name, @{Name=“Size”; Expression={Format-FileSize($_.Sum)}}
    }
}

