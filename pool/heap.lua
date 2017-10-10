-- Copyright (c) 2007-2011 Incremental IP Limited.

--[[
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
--]]


-- heap construction ---------------------------------------------------------

local heap = {}
heap.__index = heap


local function default_comparison(k1, k2)
  return k1 < k2
end


function heap:new(comparison)
  return setmetatable(
    { length = 0, comparison = comparison or default_comparison }, self)
end


-- info ----------------------------------------------------------------------

function heap:next_key()
  assert(self.length > 0, "The heap is empty")
  return self[1].key
end


function heap:empty()
  return self.length == 0
end


-- insertion and popping -----------------------------------------------------

function heap:insert(k, v)
  assert(k, "You can't insert nil into a heap")

  local cmp = self.comparison

  -- float the new key up from the bottom of the heap
  self.length = self.length + 1
  local new_record = self[self.length]  -- keep the old table to save garbage
  local child_index = self.length
  while child_index > 1 do
    local parent_index = math.floor(child_index / 2)
    local parent_rec = self[parent_index]
    if cmp(k, parent_rec.key) then
      self[child_index] = parent_rec
    else
      break
    end
    child_index = parent_index
  end
  if new_record then
    new_record.key = k
    new_record.value = v
  else
    new_record = {key = k, value = v}
  end
  self[child_index] = new_record
end


function heap:pop()
  assert(self.length > 0, "The heap is empty")

  local cmp = self.comparison

  -- pop the top of the heap
  local result = self[1]

  -- push the last element in the heap down from the top
  local last = self[self.length]
  local last_key = (last and last.key) or nil
  -- keep the old record around to save on garbage
  self[self.length] = self[1]
  self.length = self.length - 1

  local parent_index = 1
  while parent_index * 2 <= self.length do
    local child_index = parent_index * 2
    if child_index+1 <= self.length and
       cmp(self[child_index+1].key, self[child_index].key) then
      child_index = child_index + 1
    end
    local child_rec = self[child_index]
    local child_key = child_rec.key
    if cmp(last_key, child_key) then
      break
    else
      self[parent_index] = child_rec
      parent_index = child_index
    end
  end
  self[parent_index] = last
  return result.key, result.value
end


-- checking ------------------------------------------------------------------

function heap:check()
  local cmp = self.comparison
  local i = 1
  while true do
    if i*2 > self.length then return true end
    if cmp(self[i*2].key, self[i].key) then return false end
    if i*2+1 > self.length then return true end
    if cmp(self[i*2+1].key, self[i].key) then return false end
    i = i + 1
  end
end


-- pretty printing -----------------------------------------------------------

function heap:write(f, tostring_func)
  f = f or io.stdout
  tostring_func = tostring_func or tostring

  local function write_node(lines, i, level, end_spaces)
    if self.length < 1 then return 0 end

    i = i or 1
    level = level or 1
    end_spaces = end_spaces or 0
    lines[level] = lines[level] or ""

    local my_string = tostring_func(self[i].key)

    local left_child_index = i * 2
    local left_spaces, right_spaces = 0, 0
    if left_child_index <= self.length then
      left_spaces = write_node(lines, left_child_index, level+1, my_string:len())
    end
    if left_child_index + 1 <= self.length then
      right_spaces = write_node(lines, left_child_index + 1, level+1, end_spaces)
    end
    lines[level] = lines[level]..string.rep(' ', left_spaces)..
                   my_string..string.rep(' ', right_spaces + end_spaces)
    return left_spaces + my_string:len() + right_spaces
  end

  local lines = {}
  write_node(lines)
  for _, l in ipairs(lines) do
    f:write(l, '\n')
  end
end


------------------------------------------------------------------------------

return heap

------------------------------------------------------------------------------
