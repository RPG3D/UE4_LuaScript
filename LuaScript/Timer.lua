

local Class = require("Class")
local Timer = Timer or Class()

function Timer:new(InName)
	local Obj = {}
	Obj.Name = InName
	Obj.TimerList = {}
	setmetatable(Obj, self)
	return Obj
end

function Timer:SetTimer(InName, InTotalTime, InInterval, InComplete, InUpdate)

    local NewTimer = {}

    NewTimer.TotalTime = InTotalTime
    NewTimer.RemainTime = InTotalTime
    NewTimer.LastUpdate = InTotalTime
    NewTimer.Interval = InInterval
    NewTimer.Complete = InComplete or nil
    NewTimer.Update = InUpdate or nil

    self.TimerList[InName] = NewTimer

end

function Timer:ClearTimer(InName)
    self.TimerList[InName] = nil
end

function Timer:FindTimer(InName)
   return self.TimerList[InName]
end

function Timer:Tick(InDeltaTime)

    for k, v in pairs(self.TimerList) do
        v.RemainTime = v.RemainTime - InDeltaTime

        if(v.RemainTime <= 0) then
            if(v.Complete) then
                v.Complete(k)
            end

            self.TimerList[k] = nil
        else
            if(v.LastUpdate - v.RemainTime >= v.Interval) then
                v.LastUpdate = v.RemainTime
                if(v.Update) then
                    v.Update(k)
                end
            end
        end

    end

end

return Timer