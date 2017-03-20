local effil = require 'effil'

function downlad_file(url)
    os.execute("curl -ss " .. url .. " -o " .. url .. ".html")
    return url .. ".html"
end

dowloads = {}
for i, url in ipairs({ "example.com", "bash.im", "reddit.com" }) do
    dowloads[i] = effil.async(downlad_file, url)
end

for i, download in ipairs(dowloads) do
    print(download:get())
end
