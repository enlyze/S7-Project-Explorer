cd $PSScriptRoot

# Set the current Git revision in version.h
$gitRevision = & git rev-parse HEAD
((Get-Content -Path src\version.h -Raw) -Replace 'unknown revision',$gitRevision) | Set-Content -Path src\version.h

# Build S7-Project-Explorer
cd src
cmd /c "C:\BuildTools\Common7\Tools\VsDevCmd.bat && msbuild S7-Project-Explorer.sln /m /t:Rebuild /p:Configuration=Release"
