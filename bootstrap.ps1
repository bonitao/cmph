$BusyboxVersion = "5857-g3681e397f"
$MiseVersion    = "2025.11.2"

if (-not (Get-Command scoop -ErrorAction SilentlyContinue)) {
  Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
  Invoke-RestMethod https://get.scoop.sh | Invoke-Expression
}

# Ensure Scoop buckets exist (main/extras)
try {
  $buckets = scoop bucket list 2>$null
  if (-not ($buckets | Select-String -SimpleMatch 'main')) {
    scoop bucket add main | Out-Null
  }
  if (-not ($buckets | Select-String -SimpleMatch 'extras')) {
    scoop bucket add extras | Out-Null
  }
} catch {
  Write-Warning "Could not verify/add Scoop buckets: $_"
}

# Ensure BusyBox via Scoop (Windows)
if (-not (Get-Command busybox -ErrorAction SilentlyContinue)) {
  Write-Host "Installing BusyBox via Scoop (version $BusyboxVersion)..."
  try {
    scoop install "busybox@$BusyboxVersion"
  } catch {
    Write-Warning "BusyBox installation failed: $_"
  }
} else {
  Write-Host "BusyBox is already installed."
}

# Ensure buf via Scoop [test-only]
if (-not (Get-Command buf -ErrorAction SilentlyContinue)) {
  $BufVersion = "1.34.0"
  Write-Host "Installing buf via Scoop (version $BufVersion)..."
  try { scoop install "buf@$BufVersion" } catch {
    Write-Warning "buf installation failed: $_"; Write-Host "Falling back to latest buf..."; try { scoop install buf } catch { Write-Warning "Fallback buf install failed: $_" }
  }
} else { Write-Host "buf is already installed." }

if (-not (Get-Command mise -ErrorAction SilentlyContinue)) {
  Write-Host "Installing Mise via Scoop (version $MiseVersion)..."
  $miseInstalled = $false
  try {
    scoop install "mise@$MiseVersion"
    if (Get-Command mise -ErrorAction SilentlyContinue) { $miseInstalled = $true }
  } catch {
    Write-Warning "Scoop install of mise@$MiseVersion failed: $_"
  }

  if (-not $miseInstalled) {
    Write-Host "Falling back to Scoop latest for mise..."
    try {
      scoop install mise
      if (Get-Command mise -ErrorAction SilentlyContinue) { $miseInstalled = $true }
    } catch {
      Write-Warning "Scoop install of mise (latest) also failed: $_"
    }
  }

  if ($miseInstalled) {
    Write-Host "Trusting all projects in the monorepo..."
    Get-ChildItem -Recurse -File -Filter ".mise.toml" | ForEach-Object {
        $dir = Split-Path $_.FullName
        mise trust -y $dir
    }

    Write-Host "Installing default tools..."
    mise install

    Write-Host "Mise installation and setup completed."
  } else {
    Write-Warning "Mise is not installed; skipping trust and default tool install."
  }
} else {
  Write-Host "Mise is already installed."
}

# Activate mise for this session
$activation = (mise activate pwsh)
if ($activation) {
  Invoke-Expression ($activation -join "`n")
  Write-Host "Mise is now active in this shell."
} else {
  Write-Warning "Mise activation returned no script. You may add 'mise activate pwsh | Invoke-Expression' to your PowerShell profile."
}

Write-Host "For more information on permanently activating Mise in PowerShell,"
Write-Host "run: 'mise activate --help'."
