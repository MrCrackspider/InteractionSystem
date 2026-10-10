// © Volga Studios. All rights reserved.

#include "InteractionModuleTypes.h"

#include "Engine/BlueprintGeneratedClass.h"

namespace UE::Interaction
{
	bool IsEventImplementedInBlueprint(const UClass* Class, const FName FunctionName)
	{
		if (Class == nullptr)
		{
			return false;
		}

		// A Blueprint override is a separate UFunction owned by the generated class; the native stub this forwards
		// from is owned by the C++ class that declared it.
		const UFunction* Function = Class->FindFunctionByName(FunctionName);
		return Function != nullptr && Function->GetOuter() != nullptr && Function->GetOuter()->IsA<UBlueprintGeneratedClass>();
	}
}
