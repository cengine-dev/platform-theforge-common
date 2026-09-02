# Write-Dds.ps1 — o escritor DDS compartilhado (task 06 deste repo).
#
# Extraido do bloco identico que fechava os tools de atlas do spaceinvaders, do
# breakout e do mario (3 copias conferidas na revisao pos-cengine-0.9.0); o
# zelda, que seria a 4a copia, e o consumidor de validacao. Os tools dos jogos
# antigos NAO migram: o atlas deles ja esta gerado e versionado.
#
# O que este arquivo escreve: DDS RGBA8 SEM compressao — magic "DDS ",
# DDS_HEADER de 124 bytes, DDS_PIXELFORMAT fourCC "DX10", DDS_HEADER_DXT10 com
# DXGI_FORMAT_R8G8B8A8_UNORM — que o tinydds do ResourceLoader do The-Forge le
# direto. A ARTE (paleta, retangulos, espelhos, celulas) e a identidade de cada
# jogo e fica no tool do jogo.
#
# Uso (dot-source do checkout irmao, como os vcxproj fazem com $(CommonRoot)):
#
#   . (Join-Path $commonRoot 'tools\Write-Dds.ps1')
#   Write-Dds -Path $outFile -Width $texW -Height $texH -Pixels $pixels

# O DXGI_FORMAT e PARAMETRO desde que o Vigil trouxe arte de fora (Blender).
#
# Os onze primeiros atlas do ecossistema foram PINTADOS a mao dentro do proprio
# tool, com as cores escolhidas OLHANDO o resultado na tela. Com o swapchain em
# COLOR_SPACE_SDR_SRGB e a textura em UNORM, o valor e codificado duas vezes (o
# shader le cru, o render target re-encoda na escrita) — e quem pinta olhando
# compensa isso sem perceber, porque escolheu a cor que ficou certa NO FIM.
#
# Arte IMPORTADA nao tem esse luxo: o PNG que sai do Blender ja vem codificado
# em sRGB, e a dupla codificacao clareia tudo visivelmente.
#
# Por isso o padrao continua 28: mudar o default mexeria na calibragem de onze
# jogos que ninguem pediu para mexer. Quem importa arte passa 29.
#
#   28 = DXGI_FORMAT_R8G8B8A8_UNORM        (padrao historico, atlas pintado)
#   29 = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB   (atlas importado de um render)
function Write-Dds([string]$Path, [int]$Width, [int]$Height, [byte[]]$Pixels, [uint32]$DxgiFormat = 28) {
    if ($Pixels.Length -ne $Width * $Height * 4) {
        throw "Write-Dds: esperava $($Width * $Height * 4) bytes RGBA ($Width x $Height), recebi $($Pixels.Length)"
    }

    $stream = [System.IO.File]::Create($Path)
    $w = New-Object System.IO.BinaryWriter($stream)
    try {
        $w.Write([uint32]0x20534444)          # magic "DDS "
        $w.Write([uint32]124)                 # dwSize
        $w.Write([uint32]0x2100F)             # flags
        $w.Write([uint32]$Height)             # dwHeight
        $w.Write([uint32]$Width)              # dwWidth
        $w.Write([uint32]($Width * 4))        # dwPitchOrLinearSize
        $w.Write([uint32]0)                   # dwDepth
        $w.Write([uint32]1)                   # dwMipMapCount
        for ($i = 0; $i -lt 11; $i++) { $w.Write([uint32]0) }  # dwReserved1[11]
        $w.Write([uint32]32)                  # DDS_PIXELFORMAT dwSize
        $w.Write([uint32]0x4)                 # DDPF_FOURCC
        $w.Write([uint32]0x30315844)          # 'DX10'
        for ($i = 0; $i -lt 5; $i++) { $w.Write([uint32]0) }
        $w.Write([uint32]0x1000)              # DDSCAPS_TEXTURE
        for ($i = 0; $i -lt 4; $i++) { $w.Write([uint32]0) }
        $w.Write([uint32]$DxgiFormat)         # DXGI_FORMAT (28 UNORM / 29 UNORM_SRGB)
        $w.Write([uint32]3)                   # TEXTURE2D
        $w.Write([uint32]0)                   # miscFlag
        $w.Write([uint32]1)                   # arraySize
        $w.Write([uint32]0)                   # miscFlags2
        $w.Write($Pixels)
    }
    finally {
        $w.Dispose()
    }

    Write-Host "OK: $Path ($((Get-Item $Path).Length) bytes - esperado $(128 + 20 + $Pixels.Length))"
}
