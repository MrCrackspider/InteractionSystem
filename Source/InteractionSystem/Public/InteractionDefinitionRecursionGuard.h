// © Volga Studios. All rights reserved.

#pragma once

#include "Containers/Array.h"

class UInteractionDefinition;

namespace UE::Interaction
{
	/** Definitions currently being walked on one thread, innermost last. @see FScopedDefinitionRecursion. */
	using FDefinitionRecursionStack = TArray<const UInteractionDefinition*, TInlineAllocator<8>>;

	/**
	 * Marks a definition as being walked for as long as it is in scope, and reports one that was already on the
	 * stack it was handed.
	 *
	 * A definition can nest one that nests it back, directly or around a longer chain, and nothing stops that being
	 * authored. Unguarded it is not a wrong answer but an infinite recursion, so every kind of nesting breaks the
	 * cycle with one of these.
	 *
	 * Each kind of nesting owns its own thread-local stack. Scoring recursion and condition recursion are
	 * independent — a definition may legitimately be nested for scoring while its conditions are being evaluated —
	 * and one shared stack would report that as a cycle.
	 */
	struct FScopedDefinitionRecursion
	{
		FScopedDefinitionRecursion(FDefinitionRecursionStack& InStack, const UInteractionDefinition* InDefinition)
			: Stack(InStack)
			, bEntered(!InStack.Contains(InDefinition))
		{
			if (bEntered)
			{
				Stack.Add(InDefinition);
			}
		}

		~FScopedDefinitionRecursion()
		{
			if (bEntered)
			{
				Stack.Pop(EAllowShrinking::No);
			}
		}

		FScopedDefinitionRecursion(const FScopedDefinitionRecursion&) = delete;
		FScopedDefinitionRecursion& operator=(const FScopedDefinitionRecursion&) = delete;

		/** @return whether the definition was already being walked, in which case the caller must not recurse. */
		bool IsCycle() const { return !bEntered; }

	private:

		FDefinitionRecursionStack& Stack;

		bool bEntered = false;
	};
}
