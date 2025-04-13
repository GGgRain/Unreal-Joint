#pragma once
#include "VoltAnimation.h"
#include "VoltAnimationManager.h"
#include "VoltModuleItem.h"
#include "VoltSubsystem.h"


FORCEINLINE TScriptInterface<IVoltInterface> VOLTCORE_API VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(
	const TWeakPtr<SWidget>& Slate)
{
	if (Slate == nullptr) return nullptr;
	if (!Slate.IsValid()) return nullptr;
	
	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		return Subsystem->FindOrAssignVoltInterfaceFor(Slate);
	}
	
	return nullptr;
}


FORCEINLINE void VOLTCORE_API VOLT_ASSIGN_INTERFACE(
	TScriptInterface<IVoltInterface> SlateInterface)
{
	if (!SlateInterface) return;

	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		Subsystem->AssignVoltInterface(SlateInterface);
	}
}





//Shared Animation Manager Related

FORCEINLINE const FVoltAnimationTrack VOLTCORE_API VOLT_PLAY_ANIM(
	const TWeakPtr<SWidget>& Slate,
	const UVoltAnimation* Animation)
{
	if (Slate == nullptr) return FVoltAnimationTrack::NullTrack;
	if (Animation == nullptr) return FVoltAnimationTrack::NullTrack;


	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager()) return AnimationManager->PlayAnimationFor(VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(Slate), Animation);
	
	return FVoltAnimationTrack::NullTrack;

}


FORCEINLINE const FVoltAnimationTrack VOLTCORE_API VOLT_PLAY_ANIM(
	TScriptInterface<IVoltInterface> SlateInterface,
	const UVoltAnimation* Animation)
{
	if (SlateInterface == nullptr) return FVoltAnimationTrack::NullTrack;
	if (Animation == nullptr) return FVoltAnimationTrack::NullTrack;
	
	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager()) return AnimationManager->PlayAnimationFor(SlateInterface, Animation);

	return FVoltAnimationTrack::NullTrack;
}


FORCEINLINE void VOLTCORE_API VOLT_STOP_ANIM(
	const FVoltAnimationTrack& Track)
{
	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager())
	{
		AnimationManager->FlushTrack(Track);
	}
}

FORCEINLINE bool VOLTCORE_API VOLT_CHECK_PLAYING_ANIM(
	const FVoltAnimationTrack& Track)
{
	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager()) AnimationManager->HasTrack(Track);

	return false;
}


FORCEINLINE void VOLTCORE_API VOLT_STOP_ALL_ANIM(
	TScriptInterface<IVoltInterface> SlateInterface)
{
	if (SlateInterface == nullptr) return;

	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager()) AnimationManager->FlushTracksFor(SlateInterface);

}

FORCEINLINE void VOLTCORE_API VOLT_STOP_ALL_ANIM(
	const TWeakPtr<SWidget>& Slate)
{
	if (Slate == nullptr) return;
	
	if(UVoltAnimationManager* AnimationManager = UVoltSubsystem::GetSharedAnimationManager()) AnimationManager->FlushTracksFor(VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(Slate));

}









//With Specific Animation Manager

FORCEINLINE const FVoltAnimationTrack VOLTCORE_API VOLT_PLAY_ANIM(
	UVoltAnimationManager* Manager,
	const TWeakPtr<SWidget>& Slate,
	const UVoltAnimation* Animation)
{
	if (Manager == nullptr) return FVoltAnimationTrack::NullTrack;
	if (!IsValid(Manager)) return FVoltAnimationTrack::NullTrack;
	if (!Manager->IsValidLowLevel()) return FVoltAnimationTrack::NullTrack;
	if (Slate == nullptr) return FVoltAnimationTrack::NullTrack;
	if (Animation == nullptr) return FVoltAnimationTrack::NullTrack;


	return Manager->PlayAnimationFor(
		VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(Slate),
		Animation);
}


FORCEINLINE const FVoltAnimationTrack VOLTCORE_API VOLT_PLAY_ANIM(
	UVoltAnimationManager* Manager,
	TScriptInterface<IVoltInterface> SlateInterface,
	const UVoltAnimation* Animation)
{
	if (Manager == nullptr) return FVoltAnimationTrack::NullTrack;
	if (!IsValid(Manager)) return FVoltAnimationTrack::NullTrack;
	if (!Manager->IsValidLowLevel()) return FVoltAnimationTrack::NullTrack;
	if (SlateInterface == nullptr) return FVoltAnimationTrack::NullTrack;
	if (Animation == nullptr) return FVoltAnimationTrack::NullTrack;
	
	return Manager->PlayAnimationFor(SlateInterface, Animation);
}

FORCEINLINE void VOLTCORE_API VOLT_STOP_ANIM(
	UVoltAnimationManager* Manager,
	const FVoltAnimationTrack& Track)
{
	if (Manager == nullptr) return;
	if (!IsValid(Manager)) return;
	if (!Manager->IsValidLowLevel()) return;

	Manager->FlushTrack(Track);
	
}

FORCEINLINE bool VOLTCORE_API VOLT_CHECK_PLAYING_ANIM(
	UVoltAnimationManager* Manager,
	const FVoltAnimationTrack& Track)
{
	if (Manager == nullptr) return false;
	if (!IsValid(Manager)) return false;
	if (!Manager->IsValidLowLevel()) return false;

	return Manager->HasTrack(Track);
}


FORCEINLINE void VOLTCORE_API VOLT_STOP_ALL_ANIM(
	UVoltAnimationManager* Manager,
	TScriptInterface<IVoltInterface> SlateInterface)
{
	if (Manager == nullptr) return;
	if (!IsValid(Manager))  return;
	if (!Manager->IsValidLowLevel()) return;
	if (SlateInterface == nullptr) return;

	Manager->FlushTracksFor(SlateInterface);
}

FORCEINLINE void VOLTCORE_API VOLT_STOP_ALL_ANIM(
	UVoltAnimationManager* Manager,
	const TWeakPtr<SWidget>& Slate)
{
	if (Manager == nullptr) return;
	if (!IsValid(Manager)) return;
	if (!Manager->IsValidLowLevel()) return;
	if (Slate == nullptr) return;

	Manager->FlushTracksFor(VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(Slate));
}


/**
 * Implement a new animation manager.
 * Volt 1.1: Now you must provide one of those : a valid outer other than UVoltSubsystem or a valid VoltInterface or any owner slate ptr. (shared pointer, use "Slate"->AsShared)
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param Outer An outer of the new animation manager. If it was nullptr, then the Volt subsystem will be used as its outer.
 * @param OwnerVoltInterface An owner volt interface of the newly implemented animation manager.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	UVoltAnimationManager** ManagerPtr, UObject* Outer, TScriptInterface<IVoltInterface> OwnerVoltInterface)
{
	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		UVoltAnimationManager* NewManager = nullptr;

		if (Outer != nullptr && Outer->IsValidLowLevel()) NewManager = NewObject<UVoltAnimationManager>(Outer);

		if (NewManager == nullptr) NewManager = NewObject<UVoltAnimationManager>(Subsystem);
		
		if (OwnerVoltInterface != nullptr) NewManager->SetOwnerVoltInterface(OwnerVoltInterface);
		
		if (NewManager != nullptr)
		{
			Subsystem->RegisterAnimationManager(NewManager);
			(*ManagerPtr) = NewManager;
		}
		
	}
}

/**
 * Implement a new animation manager.
 * Volt 1.1: Now you must provide one of those : a valid outer other than UVoltSubsystem or a valid VoltInterface or any owner slate ptr. (shared pointer, use "Slate"->AsShared)
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param Outer An outer of the new animation manager. If it was nullptr, then the Volt subsystem will be used as its outer.
 * @param OwnerVoltInterface An owner volt interface of the newly implemented animation manager.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	TObjectPtr<UVoltAnimationManager>* ManagerPtr, UObject* Outer, TScriptInterface<IVoltInterface> OwnerVoltInterface)
{
	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		UVoltAnimationManager* NewManager = nullptr;

		if (Outer != nullptr && Outer->IsValidLowLevel()) NewManager = NewObject<UVoltAnimationManager>(Outer);

		if (NewManager == nullptr) NewManager = NewObject<UVoltAnimationManager>(Subsystem);
		
		if (OwnerVoltInterface != nullptr) NewManager->SetOwnerVoltInterface(OwnerVoltInterface);
		
		if (NewManager != nullptr)
		{
			Subsystem->RegisterAnimationManager(NewManager);
			(*ManagerPtr) = NewManager;
		}
	}
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param Outer An outer of the new animation manager. If it was nullptr, then the Volt subsystem will be used as its outer.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	UVoltAnimationManager** ManagerPtr, UObject* Outer)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr,Outer,nullptr);
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param Outer An outer of the new animation manager. If it was nullptr, then the Volt subsystem will be used as its outer.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	TObjectPtr<UVoltAnimationManager>* ManagerPtr, UObject* Outer)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr, Outer,nullptr);
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param OwnerVoltInterface An owner volt interface of the newly implemented animation manager.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	TObjectPtr<UVoltAnimationManager>* ManagerPtr, TScriptInterface<IVoltInterface> OwnerVoltInterface)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr, nullptr, OwnerVoltInterface);
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param OwnerVoltInterface An owner volt interface of the newly implemented animation manager.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	UVoltAnimationManager** ManagerPtr, TScriptInterface<IVoltInterface> OwnerVoltInterface)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr, nullptr, OwnerVoltInterface);
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param OwnerSlate An owner slate of the newly implemented animation manager. The system will create a UVoltProxy to wrap the slate instance.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	TObjectPtr<UVoltAnimationManager>* ManagerPtr, const TWeakPtr<SWidget>& OwnerSlate)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr, nullptr, VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(OwnerSlate));
}

/**
 * Implement a new animation manager.
 * @param ManagerPtr A pointer to the property that will store new animation manager object.
 * @param OwnerSlate An owner slate of the newly implemented animation manager. The system will create a UVoltProxy to wrap the slate instance.
 */
FORCEINLINE void VOLTCORE_API VOLT_IMPLEMENT_MANAGER(
	UVoltAnimationManager**  ManagerPtr, const TWeakPtr<SWidget>& OwnerSlate)
{
	VOLT_IMPLEMENT_MANAGER(ManagerPtr, nullptr, VOLT_FIND_OR_ASSIGN_INTERFACE_FOR(OwnerSlate));
}




FORCEINLINE void VOLTCORE_API VOLT_RELEASE_MANAGER(
	UVoltAnimationManager** ManagerPtr)
{
	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		if ((*ManagerPtr) == nullptr) return;
		
		if (!(*ManagerPtr)->IsValidLowLevel()) return;

		Subsystem->UnregisterAnimationManager(*ManagerPtr);

		(*ManagerPtr)->ReleaseAll();

		(*ManagerPtr) = nullptr;
	}
}


FORCEINLINE void VOLTCORE_API VOLT_RELEASE_MANAGER(
	TObjectPtr<UVoltAnimationManager>* ManagerPtr)
{
	if (UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); Subsystem != nullptr)
	{
		if ((*ManagerPtr) == nullptr) return;
		
		if (!(*ManagerPtr)->IsValidLowLevel()) return;
		
		Subsystem->UnregisterAnimationManager(*ManagerPtr);

		(*ManagerPtr)->ReleaseAll();

		(*ManagerPtr) = nullptr;
	}
}

template<typename AnimType = UVoltAnimation>
FORCEINLINE AnimType* VOLT_GET_ANIMATION(UObject* Owner = GetTransientPackage())
{
	return NewObject<AnimType>(Owner,AnimType::StaticClass());
}

template<typename AnimType = UVoltAnimation>
FORCEINLINE AnimType* VOLT_GET_ANIMATION(TSubclassOf<UVoltAnimation> InAnimType, UObject* Owner = GetTransientPackage())
{
	return NewObject<AnimType>(Owner, InAnimType);
}











/**
 * Bunch of declarations for the declarative animation making syntax.
 */



//Animation Related

template<class AnimationType = UVoltAnimation>
struct TVoltAnimationDecl
{

	TVoltAnimationDecl(UObject* Parent = nullptr) : _Animation( nullptr )
	{
		_Animation = NewObject<AnimationType>(Parent ? Parent : GetTransientPackage());
	}
	
	/**
	 * Attach modules to the animation.
	 * @param InModules Modules to attach.
	 * @return A reference to the widget that we constructed.
	 */
	template<typename... Modules>
	AnimationType* operator()(Modules*... InModules) const
	{
		const int Dummy[] = { 0, (_Animation->AddModule(InModules), 0)... };
		
		return _Animation;
	}
	
	TObjectPtr<UVoltAnimation> _Animation;
	
};


/**
 * Declare a new animation from specified animation class type.
 */
#define VOLT_MAKE_ANIMATION_WITH_TYPE( AnimationType ) \
	TVoltAnimationDecl<AnimationType>()

/**
 * Declare a new animation.
 * 
 * Volt 1.2: if you want to start to make animation with specific UVoltAnimation class, Use VOLT_MAKE_ANIMATION_WITH_TYPE instead.
 */
#define VOLT_MAKE_ANIMATION( ... ) \
	TVoltAnimationDecl<UVoltAnimation>()


//Module Related

/** Base class for module decls. It supports some basic functionalities for the chaining and children feeding*/
struct FVoltModuleBaseModuleDecl
{
	explicit FVoltModuleBaseModuleDecl() = default;
};

template<class ModuleType>
struct TVoltModuleDecl : public FVoltModuleBaseModuleDecl
{
	explicit TVoltModuleDecl(UObject* Parent = nullptr) : _Module( nullptr )
	{
		_Module = NewObject<ModuleType>(Parent ? Parent : GetTransientPackage());
	}
	
	/**
	 * Complete module construction from InArgs.
	 * @param InArgs  NamedArguments from which to construct the widget.
	 * @return A reference to the widget that we constructed.
	 */
	ModuleType* operator<<=( const typename ModuleType::FArguments& InArgs ) const
	{
		//Make the argument instance to construct the necessary data for the module.
		_Module->Construct(InArgs);
		
		return _Module;
	}

	TObjectPtr<ModuleType> _Module;

};

//Make volt module. This feature requires 'void Construct(const FArguments& Args);' to be declared on the module class.
#define VOLT_MAKE_MODULE( ModuleType, ... ) \
	TVoltModuleDecl<ModuleType>() <<= ModuleType::FArguments()\




//Module Argument Related

/** Base class for named arguments.*/
struct FVoltModuleBaseNamedArgs
{
	FVoltModuleBaseNamedArgs() = default;
};

/** Base class for named arguments. */
template<typename VoltModuleType>
struct TVoltModuleBaseNamedArgs : public FVoltModuleBaseNamedArgs
{
	typedef typename VoltModuleType::FArguments VoltModuleArgsType;

	/** Used by the named argument pattern as a safe way to 'return *this' for call-chaining purposes. */
	VoltModuleArgsType& Me()
	{
		return *(static_cast<VoltModuleArgsType*>(this));
	}
};

// ModuleArgsType is a super-name for all the declarations that refers to the argument structure itself.
#define VOLT_MODULE_BEGIN_ARGS( ModuleType ) \
	public: \
	struct FArguments : public TVoltModuleBaseNamedArgs<ModuleType> \
	{ \
		typedef FArguments ModuleArgsType; \
		FORCENOINLINE FArguments()

#define VOLT_MODULE_END_ARGS() \
};

#define VOLT_MODULE_PRIVATE_ARGUMENT_VARIABLE( ArgType, ArgName ) \
	ArgType _##ArgName

#define VOLT_MODULE_PRIVATE_ARGUMENT_FUNCTION( ArgType, ArgName ) \
	ModuleArgsType& ArgName( ArgType InArg ) \
	{ \
		_##ArgName = InArg; \
		return static_cast<ModuleArgsType*>(this)->Me(); \
	}

#define VOLT_MODULE_ARGUMENT( ArgType, ArgName ) \
	VOLT_MODULE_PRIVATE_ARGUMENT_VARIABLE( ArgType, ArgName ); \
	VOLT_MODULE_PRIVATE_ARGUMENT_FUNCTION ( ArgType, ArgName )

/**
 * Use this macro between VOLT_MODULE_BEGIN_ARGS and VOLT_MODULE_END_ARGS
 * in order to add support for slots with the construct pattern.
 */
#define VOLT_SUBMODULE_CONTAINER_ARGUMENT(SlotName)\
	TArray<UVoltModuleItem*> _##SlotName; \
	template<typename... Modules> \
	ModuleArgsType& operator()(Modules*... InModules) \
	{ \
		const int Dummy[] = { 0, (_##SlotName.Add(InModules), 0)... }; \
		return static_cast<ModuleArgsType*>(this)->Me(); \
	} \











//Volt Variable related

/**
 * Helper macro for declaring getter function for specified property.
 * @param VariableType type of the property.
 * @param VariableName name of the property.
 */
#define MACRO_DEF_GETTER_FUNCTION( VariableType, VariableName ) \
	VariableType Get_##VariableName() { \
		return VariableName;\
	}
