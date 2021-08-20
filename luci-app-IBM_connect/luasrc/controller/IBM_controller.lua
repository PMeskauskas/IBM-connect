module("luci.controller.IBM_controller", package.seeall)

function index()
        entry({"admin", "services", "IBM"}, cbi("IBM_model"), "IMB connect",100)
end