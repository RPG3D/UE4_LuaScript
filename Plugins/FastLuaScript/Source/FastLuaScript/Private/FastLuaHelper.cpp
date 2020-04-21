// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaHelper.h"
#include "UObject/StructOnScope.h"
#include "CoreUObject.h"
#include "lua/lua.hpp"
#include "lua/lstate.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaScript.h"
#include "FastLuaDelegate.h"
#include "FastLuaStat.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LuaDelegateWrapper.h"
#include "LuaObjectWrapper.h"
#include "ILuaWrapper.h"
#include "LuaStructWrapper.h"
#include "Kismet/GameplayStatics.h"
#include "LuaLatentActionWrapper.h"

static FName LatentPropName = FName("LatentInfo");

FString FastLuaHelper::GetPropertyTypeName(const FProperty* InProp)
{
	FString TypeName;
	if (!InProp)
	{
		return TypeName;
	}

	TypeName = InProp->GetCPPType();

	if (const FClassProperty* ClassProp = CastField<FClassProperty>(InProp))
	{
		FString MetaClassName = ClassProp->MetaClass->GetName();
		FString MetaClassPrefix = ClassProp->MetaClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("TSubclassOf<%s%s>"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		TypeName = FString::Printf(TEXT("TScriptInterface<I%s>"), *MetaClassName);
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FString ElementTypeName = ArrayProp->Inner->GetCPPType();
		TypeName = FString::Printf(TEXT("TArray<%s>"), *ElementTypeName);
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(InProp))
	{
		FString ElementTypeName = SetProp->ElementProp->GetCPPType();
		TypeName = FString::Printf(TEXT("TSet<%s>"), *ElementTypeName);
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(InProp))
	{
		FString KeyTypeName = MapProp->KeyProp->GetCPPType();
		FString ValueTypeName = MapProp->ValueProp->GetCPPType();

		TypeName = FString::Printf(TEXT("TMap<%s, %s>"), *KeyTypeName, *ValueTypeName);
	}
	else if (const FWeakObjectProperty* WeakObjectProp = CastField<FWeakObjectProperty>(InProp))
	{
		FString MetaClassName = WeakObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = WeakObjectProp->PropertyClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FString FuncName = DelegateProp->SignatureFunction->GetPrefixCPP() + DelegateProp->SignatureFunction->GetName();
		FString OutName = DelegateProp->SignatureFunction->GetOuter()->GetName();
		if (OutName.Len() > 3)
		{
			FuncName = OutName + FString("::") + FuncName;
		}
		TypeName = FuncName;
	}
	else if (const FMulticastDelegateProperty* MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FString FuncName = MultiDelegateProp->SignatureFunction->GetPrefixCPP() + MultiDelegateProp->SignatureFunction->GetName();
		FString OutName = MultiDelegateProp->SignatureFunction->GetOuter()->GetName();
		if (OutName.Len() > 3)
		{
			FuncName = OutName + FString("::") + FuncName;
		}
		TypeName = FuncName;
	}
	else if (const FFieldPathProperty* FieldPathProp = CastField<FFieldPathProperty>(InProp))
	{
		FString MetaClassName = FieldPathProp->PropertyClass->GetName();
		//FString MetaClassPrefix = FieldPathProp->PropertyClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("%s"), *MetaClassName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid property type? %s"), *InProp->GetNameCPP());
	}
	return TypeName;
}


void FastLuaHelper::PushProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InArrayElementIndex)
{
	if (InProp == nullptr || InBuff == nullptr)
	{
		return;
	}

	if (const FNumericProperty* NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			lua_pushinteger(InL, NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex)));
		}
		else
		{
			lua_pushnumber(InL, NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex)));
		}
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		const FNumericProperty* NumEnumProp = EnumProp->GetUnderlyingProperty();
		lua_pushinteger(InL, NumEnumProp ? NumEnumProp->GetSignedIntPropertyValue(InBuff) : 0);
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		lua_pushboolean(InL, BoolProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		FName name = NameProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*name.ToString()));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		const FString& str = StrProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*str));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		const FText& text = TextProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*text.ToString()));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ClassProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		if (const UScriptStruct * ScriptStruct = Cast<UScriptStruct>(StructProp->Struct))
		{
			FLuaStructWrapper::PushStruct(InL, ScriptStruct, StructProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex));
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ObjectProp->GetObjectPropertyValue(ObjectProp->GetPropertyValuePtr_InContainer(InBuff, InArrayElementIndex)));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		void* ValuePtr = const_cast<TScriptDelegate<FWeakObjectPtr>*>(DelegateProp->GetPropertyValuePtr_InContainer(InBuff, InArrayElementIndex));
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, false, DelegateProp->SignatureFunction);
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		void* ValuePtr = MultiDelegateProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex);
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, true, MultiDelegateProp->SignatureFunction);
	}
	else if (const FArrayProperty * ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			PushProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < SetHelper.Num(); ++i)
		{
			PushProperty(InL, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const FMapProperty * MapProp = CastField<FMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < MapHelper.Num(); ++i)
		{
			uint8* PairPtr = MapHelper.GetPairPtr(i);
			PushProperty(InL, MapProp->KeyProp, PairPtr);
			PushProperty(InL, MapProp->ValueProp, PairPtr);
			lua_rawset(InL, -3);
		}
	}
}

void FastLuaHelper::FetchProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InStackIndex, int32 InArrayElementIndex)
{
	//no enough params
	if (InBuff == nullptr || lua_gettop(InL) < lua_absindex(InL, InStackIndex))
	{
		return;
	}

	FString PropName = InProp->GetName();

	if (const FNumericProperty * NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			int64 TmpVal = lua_tointeger(InL, InStackIndex);
			NumProp->SetIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff), TmpVal);
		}
		else
		{
			float TmpVal = lua_tonumber(InL, InStackIndex);
			NumProp->SetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff), TmpVal);
		}
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		BoolProp->SetPropertyValue_InContainer(InBuff, (bool)lua_toboolean(InL, InStackIndex));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		NameProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		StrProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		TextProp->SetPropertyValue_InContainer(InBuff, FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex))));
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(InBuff), lua_tointeger(InL, InStackIndex));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		//TODO check property
		ClassProp->SetPropertyValue_InContainer(InBuff, FLuaObjectWrapper::FetchObject(InL, InStackIndex, true));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		void* Data = FLuaStructWrapper::FetchStruct(InL, InStackIndex, StructProp->Struct->GetStructureSize());
		if (Data)
		{
			StructProp->Struct->CopyScriptStruct(StructProp->ContainerPtrToValuePtr<void>(InBuff), Data);
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		//TODO, check property
		ObjectProp->SetObjectPropertyValue_InContainer(InBuff, FLuaObjectWrapper::FetchObject(InL, InStackIndex));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && !Wrapper->IsMulti())
		{
			DelegateProp->SetPropertyValue_InContainer(InBuff, *(FScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && Wrapper->IsMulti())
		{
			MultiDelegateProp->SetMulticastDelegate(MultiDelegateProp->ContainerPtrToValuePtr<void>(InBuff), *(FMulticastScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (ArrayHelper.Num() < i + 1)
				{
					ArrayHelper.AddValue();
				}

				FetchProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i), -1);
				lua_pop(InL, 1);
				++i;
			}
		}
		
	}

	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (SetHelper.Num() < i + 1)
				{
					SetHelper.AddDefaultValue_Invalid_NeedsRehash();
				}

				FetchProperty(InL, SetHelper.ElementProp, SetHelper.GetElementPtr(i), -1);
				++i;
				lua_pop(InL, 1);
			}

		}
		
		SetHelper.Rehash();
	}
	else if (const FMapProperty * MapProp = CastField<FMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (MapHelper.Num() < i + 1)
				{
					MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				}

				FetchProperty(InL, MapProp->KeyProp, MapHelper.GetPairPtr(i), -2);
				FetchProperty(InL, MapProp->ValueProp, MapHelper.GetPairPtr(i), -1);
				++i;
				lua_pop(InL, 1);
			}
		}

		MapHelper.Rehash();
	}

	return;
}

int32 FastLuaHelper::CallUnrealFunction(lua_State* InL)
{
	//SCOPE_CYCLE_COUNTER(STAT_LuaCallBP);
	UFunction* Func = (UFunction*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Obj = nullptr;

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		Obj = Wrapper->GetObject();
	}
	int32 StackTop = 2;
	if (Obj == nullptr)
	{
		lua_pushnil(InL);
		return 1;
	}

	if (Func->NumParms < 1)
	{
		Obj->ProcessEvent(Func, nullptr);
		return 0;
	}
	else
	{
		static const int32 FunctionParamDataSize = 1024;
		uint8 FuncParam[FunctionParamDataSize];
		FMemory::Memzero(FuncParam, FunctionParamDataSize);
		if (Func->GetPropertiesSize() > FunctionParamDataSize)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFunction parameter data size more than 512!"));
			return 0;
		}
		FProperty* ReturnProp = nullptr;

		FStructProperty* LatentProp = nullptr;

		for (TFieldIterator<FProperty> It(Func); It; ++It)
		{
			FProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProp = Prop;
			}
			else
			{
				FastLuaHelper::FetchProperty(InL, Prop, FuncParam, StackTop++);

				if (Prop->GetFName() == LatentPropName)
				{
					LatentProp = (FStructProperty*)Prop;
				}
			}
		}

		//fix up FLatentActionInfo parameter of function
		if (LatentProp)
		{
			if (lua_pushthread(InL) == 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("never use latent in main thread!"));
				return 0;
			}

			FLatentActionInfo LatentInfo;
			ULuaLatentActionWrapper* LatentWrapper = NewObject<ULuaLatentActionWrapper>(GetTransientPackage());
			LatentWrapper->AddToRoot();
			LatentInfo.CallbackTarget = LatentWrapper;
			LatentWrapper->MainThread = InL->l_G->mainthread;
			LatentWrapper->WorkerThread = InL;
			LatentInfo.ExecutionFunction = LatentWrapper->GetWrapperFunctionName();
			//store current worker thread.
			LatentInfo.Linkage = luaL_ref(InL, LUA_REGISTRYINDEX);
			LatentInfo.UUID = GetTypeHash(FGuid::NewGuid());

			LatentProp->CopySingleValue(LatentProp->ContainerPtrToValuePtr<void>(FuncParam), &LatentInfo);
		}

		Obj->ProcessEvent(Func, FuncParam);

		int32 ReturnNum = 0;
		if (ReturnProp)
		{
			FastLuaHelper::PushProperty(InL, ReturnProp, FuncParam);
			++ReturnNum;
		}

		if (Func->HasAnyFunctionFlags(FUNC_HasOutParms))
		{
			for (TFieldIterator<FProperty> It(Func); It; ++It)
			{
				FProperty* Prop = *It;
				if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
				{
					FastLuaHelper::PushProperty(InL, *It, FuncParam);
					++ReturnNum;
				}
			}
		}
		if (LatentProp == nullptr)
		{
			return ReturnNum;
		}
		else
		{
			return lua_yield(InL, ReturnNum);
		}
	}

}

void FastLuaHelper::CallLuaFunction(UObject* Context, FFrame& TheStack, RESULT_DECL)
{
	UClass* TmpClass = Context->GetClass();

	UGameInstance* GI = UGameplayStatics::GetGameInstance(Context);

	FastLuaUnrealWrapper* LuaUnreaWrapper = *FastLuaUnrealWrapper::LuaStateMap.Find(GI);
	lua_State* LuaState = LuaUnreaWrapper ? LuaUnreaWrapper->GetLuaSate() : nullptr;
	if (LuaState == nullptr)
	{
		return;
	}
	int32 tp = lua_gettop(LuaState);
	luaL_getmetatable(LuaState, "HookUE");
	if (lua_istable(LuaState, -1) == false)
	{
		return;
	}
	lua_rawgetp(LuaState, -1, TheStack.Node);
	if (lua_isfunction(LuaState, -1))
	{
		int32 ParamsNum = 0;
		FProperty* ReturnParam = nullptr;
		//store param from UE script VM stack
		static const int32 FunctionParamDataSize = 1024;
		uint8 FuncParam[FunctionParamDataSize];
		FMemory::Memzero(FuncParam, FunctionParamDataSize);
		if (TheStack.Node->GetPropertiesSize() > FunctionParamDataSize)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFunction parameter data size more than 512!"));
		}
		//push self
		FLuaObjectWrapper::PushObject(LuaState, Context);
		++ParamsNum;

		for (TFieldIterator<FProperty> It(TheStack.Node); It; ++It)
		{
			//get function return Param
			FProperty* CurrentParam = *It;
			void* LocalValue = CurrentParam->ContainerPtrToValuePtr<void>(FuncParam);
			TheStack.StepCompiledIn<FProperty>(LocalValue);
			if (CurrentParam->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnParam = CurrentParam;
			}
			else
			{
				//set params for lua function
				FastLuaHelper::PushProperty(LuaState, CurrentParam, FuncParam, 0);
				++ParamsNum;
			}
		}

		//call lua function
		int32 CallRet = lua_pcall(LuaState, ParamsNum, ReturnParam ? 1 : 0, 0);
		if (CallRet)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(LuaState, -1)));
		}

		if (ReturnParam)
		{
			//get function return Value, in common
			FastLuaHelper::FetchProperty(LuaState, ReturnParam, FuncParam, -1);
		}
	}

	lua_settop(LuaState, tp);
}

static void UnhookAllUFunction(lua_State* InL)
{
	if (InL == nullptr)
	{
		return;
	}

	luaL_getmetatable(InL, "HookUE");
	if (lua_istable(InL, -1) == false)
	{
		return;
	}

	lua_pushnil(InL);
	while (lua_next(InL, -2))
	{
		UFunction* Func = (UFunction*)lua_touserdata(InL, -2);
		if (Func)
		{
			Func->SetNativeFunc(nullptr);
		}
		if (Func && Func->HasAnyFlags(RF_Transient))
		{
			Func->GetOwnerClass()->RemoveFunctionFromFunctionMap(Func);
			Func->MarkPendingKill();

			lua_pushnil(InL);
			lua_rawsetp(InL, -4, Func);
		}

		lua_pop(InL, 1);
	}
}


int FastLuaHelper::HookUFunction(lua_State* InL)
{
	static bool bIsBindToReset = false;
	if (bIsBindToReset == false)
	{
		FastLuaUnrealWrapper::OnLuaUnrealReset.AddLambda([InL](lua_State* InLua)
			{
				if (InL != InLua)
				{
					return;
				}
				UnhookAllUFunction(InLua);
			});

		bIsBindToReset = true;
	}

	luaL_newmetatable(InL, "HookUE");
	int32 HookTableIndex = lua_gettop(InL);

	UClass* Cls = FLuaObjectWrapper::FetchObject(InL, 1, false)->GetClass();
	FString ClsName = Cls->GetName();
	const char* ClsName_C = TCHAR_TO_UTF8(*ClsName);
	{
		lua_getglobal(InL, "require");
		lua_pushstring(InL, ClsName_C);
		int32 status = lua_pcall(InL, 1, 1, 0);  /* call 'require(name)' */
		if (status != LUA_OK)
		{
			FString ErrStr = UTF8_TO_TCHAR(lua_tostring(InL, -1));
			UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrStr);
		}
	}

	if (!lua_istable(InL, -1))
	{
		lua_pushnil(InL);
		return 1;
	}

	lua_pushnil(InL);
	while (lua_next(InL, -2))
	{
		FString FuncName = lua_tostring(InL, -2);
		UFunction* FoundFunc = Cls->FindFunctionByName(*FuncName, EIncludeSuperFlag::ExcludeSuper);
		if (FoundFunc)
		{
			FoundFunc->FunctionFlags |= FUNC_Native;
			FoundFunc->SetNativeFunc(FastLuaHelper::CallLuaFunction);
		}
		else
		{
			UFunction* FoundSuperFunc = Cls->FindFunctionByName(*FuncName, EIncludeSuperFlag::IncludeSuper);
			if (FoundSuperFunc)
			{
				FoundFunc = Cast<UFunction>(StaticDuplicateObject(FoundSuperFunc, Cls, *FuncName, RF_Public | RF_Transient));
				FoundFunc->FunctionFlags |= FUNC_Native;
				FoundFunc->SetNativeFunc(FastLuaHelper::CallLuaFunction);
				Cls->AddFunctionToFunctionMap(FoundFunc, *FuncName);
				FoundFunc->StaticLink(true);
			}
		}

		if (FoundFunc)
		{
			lua_rawsetp(InL, HookTableIndex, FoundFunc);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}

	lua_pushboolean(InL, 1);
	return 1;
}

void FastLuaHelper::FixClassMetatable(lua_State* InL, TArray<const UClass*> InRegistedClassList)
{
	for (int32 i = 0; i < InRegistedClassList.Num(); ++i)
	{
		FString ClassName = InRegistedClassList[i]->GetName();
		luaL_getmetatable(InL, TCHAR_TO_UTF8(*ClassName));
		
		UClass* SuperClass = InRegistedClassList[i]->GetSuperClass();
		if (SuperClass)
		{
			FString SuperClassName = SuperClass->GetName();
			if (luaL_getmetatable(InL, TCHAR_TO_UTF8(*ClassName)))
			{
				lua_setmetatable(InL, -2);
			}
			else
			{
				lua_pop(InL, 1);
				//当前类的父类没有被导出
				luaL_setmetatable(InL, FLuaObjectWrapper::GetMetatableName());
			}
		}
		else
		{
			//当前类没有父类，理论上只有UObject
			luaL_setmetatable(InL, FLuaObjectWrapper::GetMetatableName());
		}

		lua_pop(InL, 1);
	}
}


int FastLuaHelper::PrintLog(lua_State* L)
{
	FString StringToPrint;
	int Num = lua_gettop(L);
	for (int i = 1; i <= Num; ++i)
	{
		if (lua_isstring(L, i))
		{
			StringToPrint.Append(UTF8_TO_TCHAR(lua_tostring(L, i)));
		}
		else if (lua_isinteger(L, i) || lua_isboolean(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("%d"), lua_tointeger(L, i)));
		}
		else if (lua_isnumber(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("%f"), lua_tonumber(L, i)));
		}
		else if (lua_isuserdata(L, i) || lua_islightuserdata(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("UserData:%d"), lua_touserdata(L, i)));
		}

		if (Num > 1 && i < Num)
		{
			StringToPrint.Append(FString(","));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *StringToPrint);
	return 0;
}

