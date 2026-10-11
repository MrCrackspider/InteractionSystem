#pragma once

#include "Modules/ModuleManager.h"

class FInteractionSystemEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	/** Needs the editor engine, which does not exist yet when this module starts up at its Default loading phase. */
	void RegisterComponentVisualizers();
	void UnregisterComponentVisualizers();

	TArray<FName> RegisteredVisualizerClassNames;
};
