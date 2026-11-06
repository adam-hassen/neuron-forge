// Neuron Forge - ENSIM
//
// Utilitaires partagés par les tests d'automatisation.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace NFTestUtils
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ProductFilter;

	/** Monde de jeu temporaire, prêt à jouer, détruit à la fin du test. */
	struct FScopedTestWorld
	{
		UWorld* World = nullptr;

		explicit FScopedTestWorld(const TCHAR* Name = TEXT("NFTestWorld"))
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false, Name);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			// Sans GameMode, rien n'appelle NotifyBeginPlay (normalement fait par le GameState) :
			// on le fait ici pour que les acteurs reçoivent BeginPlay et tickent.
			if (!World->GetBegunPlay())
			{
				World->GetWorldSettings()->NotifyBeginPlay();
			}
		}

		~FScopedTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		FScopedTestWorld(const FScopedTestWorld&) = delete;
		FScopedTestWorld& operator=(const FScopedTestWorld&) = delete;

		/** Fait avancer le monde d'une frame (acteurs, composants, physique). */
		void Step(float DeltaSeconds = 1.f / 60.f)
		{
			// Une fonction de tick ne s'exécute qu'une fois par valeur de GFrameCounter,
			// normalement incrémenté par la boucle moteur : on simule une nouvelle frame.
			++GFrameCounter;
			World->Tick(LEVELTICK_All, DeltaSeconds);
		}

		/** Avance jusqu'à ce que Predicate soit vrai, au plus MaxFrames frames. Renvoie vrai si atteint. */
		template <typename PredicateType>
		bool StepUntil(PredicateType Predicate, int32 MaxFrames, float DeltaSeconds = 1.f / 60.f)
		{
			for (int32 Frame = 0; Frame < MaxFrames; ++Frame)
			{
				if (Predicate())
				{
					return true;
				}
				Step(DeltaSeconds);
			}
			return Predicate();
		}
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
