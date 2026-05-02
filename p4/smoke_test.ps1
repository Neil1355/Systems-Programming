$ErrorActionPreference = 'Stop'

$port = 5555

function New-Conn {
    param([string]$Name)
    $client = [System.Net.Sockets.TcpClient]::new('127.0.0.1', $port)
    $client.NoDelay = $true
    [pscustomobject]@{
        Name = $Name
        Client = $client
        Stream = $client.GetStream()
    }
}

function Send-Frame {
    param(
        $Conn,
        [string]$Code,
        [string]$Body
    )
    $frame = "1|$Code|$([System.Text.Encoding]::ASCII.GetByteCount($Body))|$Body"
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($frame)
    $Conn.Stream.Write($bytes, 0, $bytes.Length)
    $Conn.Stream.Flush()
}

function Read-Exact {
    param(
        $Stream,
        [int]$Count
    )
    $bytes = New-Object byte[] $Count
    $offset = 0
    while ($offset -lt $Count) {
        $read = $Stream.Read($bytes, $offset, $Count - $offset)
        if ($read -le 0) {
            throw 'connection closed while reading'
        }
        $offset += $read
    }
    return $bytes
}

function Read-Frame {
    param($Conn)
    $header = New-Object System.Text.StringBuilder
    $bars = 0
    while ($bars -lt 3) {
        $b = $Conn.Stream.ReadByte()
        if ($b -lt 0) {
            throw 'connection closed while reading header'
        }
        [void]$header.Append([char]$b)
        if ($b -eq 124) {
            $bars++
        }
    }
    $parts = $header.ToString().Split('|')
    $bodyLen = [int]$parts[2]
    $bodyBytes = Read-Exact -Stream $Conn.Stream -Count $bodyLen
    return $header.ToString() + [System.Text.Encoding]::ASCII.GetString($bodyBytes)
}

function Assert-Match {
    param(
        [string]$Actual,
        [string]$Needle,
        [string]$Label
    )
    if (-not $Actual.Contains($Needle)) {
        throw "$Label failed: expected to find '$Needle' in '$Actual'"
    }
}

$alice = New-Conn -Name 'Alice'
$bob = New-Conn -Name 'Bob'

Send-Frame $alice 'NAM' 'Alice|'
Send-Frame $bob 'NAM' 'Bob|'
$aliceWelcome = Read-Frame $alice
$bobWelcome = Read-Frame $bob
Assert-Match $aliceWelcome 'Welcome to the chat!' 'Alice welcome'
Assert-Match $bobWelcome 'Welcome to the chat!' 'Bob welcome'

Send-Frame $alice 'WHO' '#all|'
$whoAll = Read-Frame $alice
Assert-Match $whoAll 'Alice' 'WHO all'
Assert-Match $whoAll 'Bob' 'WHO all'

Send-Frame $alice 'SET' 'Smiling politely|'
$aliceStatus = Read-Frame $alice
$bobStatus = Read-Frame $bob
Assert-Match $aliceStatus 'Alice is now "Smiling politely"' 'Alice status broadcast'
Assert-Match $bobStatus 'Alice is now "Smiling politely"' 'Bob status broadcast'

Send-Frame $alice 'MSG' '|Bob|Hello, Bob!|'
$bobMessage = Read-Frame $bob
Assert-Match $bobMessage 'Hello, Bob!' 'Private message'

$alice.Client.Close()
$bob.Client.Close()
Write-Host 'smoke test passed'