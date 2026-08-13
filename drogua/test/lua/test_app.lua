assert(Drogua ~= nil)

assert(Drogua.print ~= nil)
assert(Drogua.app ~= nil)

local app = Drogua.app()

assert(app ~= nil)

assert(app.loadJsonConfig ~= nil)
assert(app.setThreadNum ~= nil)
assert(app.addListener ~= nil)
assert(app.setLogPath ~= nil)
assert(app.setLogLevel ~= nil)
assert(app.enableRunAsDaemon ~= nil)
assert(app.run ~= nil)