---@class fontatlas
local M = {}

---@class fontatlas.Builder
local C = {}
---@param format '"png"' | '"bmp"'
function C:setImageFileFormat(format) end
---@param v boolean
function C:setMultiChannelEnable(v) end
---@param name string
---@param path string
---@param face number
---@param size number
---@return boolean
function C:addFont(name, path, face, size) end
---@param name string
---@param c number
---@return boolean
function C:addCode(name, c) end
---@param name string
---@param a number
---@param b number
---@return boolean
function C:addRange(name, a, b) end
---@param name string
---@param text string
---@return boolean
function C:addText(name, text) end
---@param path string
---@param is_multi_channel boolean
---@param texture_width number
---@param texture_height number
---@param texture_edge number
---@param glyph_edge number
---@return boolean
function C:build(path, is_multi_channel, texture_width, texture_height, texture_edge, glyph_edge) end

---@return fontatlas.Builder
function M.Builder() end

return M
