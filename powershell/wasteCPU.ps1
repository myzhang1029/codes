for ($i = 0; $i -lt [Environment]::ProcessorCount; $i++) {
    Start-Job -ScriptBlock {
        while ($true) { }
    }
}

# Stop:
Get-Job | Remove-Job -Force
