$apiUrl = 'https://ci.appveyor.com/api'
$token = $Env:BearerToken
$jobId = $Env:APPVEYOR_JOB_ID
$platform = $Env:PLATFORM

Set-Location -path browser
$git_describe = git describe
Set-Location -path ..

$headers = @{
  "Authorization" = "Bearer $token"
  "Content-type" = "application/json"
}
$accountName = 'datadiode'
$projectSlug = 'sfqtwebkit'

# get project with last build details
$project = Invoke-RestMethod -Method Get -Uri "$apiUrl/projects/$accountName/$projectSlug" -Headers $headers

# replace job id with previous one
for ($i = 0; ($i -lt $project.build.jobs.length) -and ($jobId -ne $project.build.jobs[$i].jobId); $i++) {
  Write-host "Probing job $i for artifact $platform.7z"
  $jopId = $project.build.jobs[$i].jobId
  try {
    Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jopId/artifacts/$platform.7z" -OutFile ".\$platform.7z" -Headers @{ "Authorization" = "Bearer $token" }
  } catch {
    Write-host "$platform.7z not downloaded"
  }
  try {
    Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jopId/artifacts/browser%2Fbrowser_$git_describe.7z" -OutFile ".\browser\browser_$git_describe.7z" -Headers @{ "Authorization" = "Bearer $token" }
  } catch {
    Write-host "browser_$git_describe.7z not downloaded"
  }
}
