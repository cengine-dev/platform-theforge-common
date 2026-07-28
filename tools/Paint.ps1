# Paint.ps1 — os helpers de pintura dos tools de atlas (task 07 deste repo).
#
# Extraidos das copias identicas espalhadas por CINCO jogos: `Set-Pixel`
# aparecia a mao no breakout, mario-bros, zelda, starforce e delve; `Fill-Rect`
# em tres deles. Sempre a mesma coisa — escrever RGBA num `byte[]` com
# checagem de limites, e preencher retangulo chamando o primeiro.
#
# O consumidor de validacao e o Bulwark (degrau 07 daquele repo). Os cinco
# tools existentes NAO migram: o atlas deles ja esta gerado e versionado, mesma
# regra que valeu na extracao do Write-Dds.
#
# ## O que mudou na extracao
#
# As copias dependiam de `$script:texW` e `$script:pixels` — variaveis do
# escopo do script que chamava, o que so funciona por acidente de escopo e
# quebra em silencio se o tool renomear a variavel. Aqui isso virou um CANVAS
# explicito: as funcoes recebem o alvo em vez de adivinhar onde ele mora.
#
# A ARTE (paleta, celulas, retangulos) segue sendo identidade de cada jogo e
# fica no tool do jogo. Isto aqui e so o pincel.
#
# Uso (dot-source do checkout irmao, como os vcxproj fazem com $(CommonRoot)):
#
#   . (Join-Path $commonRoot 'tools\Paint.ps1')
#   . (Join-Path $commonRoot 'tools\Write-Dds.ps1')
#
#   $canvas = New-AtlasCanvas -Width 128 -Height 64
#   Fill-Rect -Canvas $canvas -X 0 -Y 0 -W 16 -H 16 -Rgb @(92, 92, 104)
#   Write-Dds -Path $out -Width $canvas.Width -Height $canvas.Height -Pixels $canvas.Pixels

function New-AtlasCanvas([int]$Width, [int]$Height) {
    if ($Width -le 0 -or $Height -le 0) {
        throw "New-AtlasCanvas: largura e altura precisam ser positivas (recebi $Width x $Height)"
    }

    # byte[] ja nasce zerado = tudo transparente. O alpha 0 e o fundo de todo
    # atlas do ecossistema: o que nao foi pintado nao aparece.
    return [pscustomobject]@{
        Width  = $Width
        Height = $Height
        Pixels = New-Object byte[] ($Width * $Height * 4)
    }
}

function Set-Pixel($Canvas, [int]$X, [int]$Y, $Rgb, [int]$Alpha = 255) {
    if ($X -lt 0 -or $Y -lt 0 -or $X -ge $Canvas.Width -or $Y -ge $Canvas.Height) { return }

    $i = ($Y * $Canvas.Width + $X) * 4
    $Canvas.Pixels[$i]     = $Rgb[0]
    $Canvas.Pixels[$i + 1] = $Rgb[1]
    $Canvas.Pixels[$i + 2] = $Rgb[2]
    $Canvas.Pixels[$i + 3] = $Alpha
}

function Fill-Rect($Canvas, [int]$X, [int]$Y, [int]$W, [int]$H, $Rgb, [int]$Alpha = 255) {
    for ($dy = 0; $dy -lt $H; $dy++) {
        for ($dx = 0; $dx -lt $W; $dx++) {
            Set-Pixel -Canvas $Canvas -X ($X + $dx) -Y ($Y + $dy) -Rgb $Rgb -Alpha $Alpha
        }
    }
}
