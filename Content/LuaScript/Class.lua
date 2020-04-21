
local Class = Class or {}

--Note:
--only single inherent, we can just use setmetatable(NewClass, ParentClass)

--local SubClass = Class(ParentClass)
function Class:__call(InParentClass)
    --declare a new Class
    local NewClass = {}

    --construct
    function NewClass:new()
        local Obj = {}
        setmetatable(Obj, NewClass)
        return Obj
    end

    --index member from parent Class
    setmetatable(NewClass, 
        { 
            __index = InParentClass
        }
    )

    --members are stored in Class table
    NewClass.__index = NewClass

    return NewClass
end

--metamethod __call is defined in Class
setmetatable(Class, Class)
Class.__index = Class

return Class