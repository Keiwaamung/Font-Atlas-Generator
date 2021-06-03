local fontatlas = require("fontatlas")

local builder = fontatlas.Builder()
builder:addFont("Sans24", "C:\\Windows\\Fonts\\msyh.ttc", 0, 24)
--builder:addFont("Sans24", "SourceHanSansSC-Regular.otf", 0, 24)
builder:addRange("Sans24", 32, 126)
builder:addRange("Sans24", 0x4E00, 0x9FFF)
builder:build("font/", false, 2048, 2048, 1, 0)
