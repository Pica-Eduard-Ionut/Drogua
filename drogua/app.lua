Dragua.print("Hello from Lua!")
Dragua.print("This message is coming from app.lua")
-- Dragua.printInt(5)


Dragua.addRoute("/drogua", "Testing lua route creation")
Dragua.registerRoute("/custom", {message="test message", extra=10, float=10.125})
