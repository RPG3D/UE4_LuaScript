

KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")

local UEMath = UEMath or {}

function UEMath.Vector(InX, InY, InZ)
	return KismetMathLibrary:MakeVector(InX, InY, InZ)
end

function UEMath.Vector2D(InX, InY)
	return KismetMathLibrary:MakeVector2D(InX, InY)
end

function UEMath.Vector4(InX, InY, InZ, InW)
	return KismetMathLibrary:MakeVector4(InX, InY, InZ, InW)
end

function UEMath.Color(InR, InG, InB, InA)
	return KismetMathLibrary:MakeColor(InR, InG, InB, InA)
end

function UEMath.Rotator(InX, InY, InZ)
	return KismetMathLibrary:MakeRotator(InX, InY, InZ)
end



return UEMath