// CaldrethImportLibrary.cpp

#include "CaldrethImportLibrary.h"

#include "CaldrethZone.h"          // ACaldrethZone, EZoneType (runtime module)
#include "CaldrethPOIMarker.h"     // ACaldrethPOIMarker (runtime module)
#include "Editor.h"                // GEditor
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Landscape.h"                 // ALandscape, ALandscape::Import
#include "LandscapeProxy.h"            // FLandscapeImportLayerInfo
#include "LandscapeInfo.h"             // ULandscapeInfo
#include "LandscapeImportHelper.h"     // ELandscapeImportAlphamapType

#define LOCTEXT_NAMESPACE "CaldrethImport"

// Editor-only log category (runtime LogCaldreth is private to the Stan_Pierwotny module).
DEFINE_LOG_CATEGORY(LogCaldrethImport);

int32 UCaldrethImportLibrary::ImportCaldrethZones(
	FString BiomePngPath,
	UDataTable* ZoneTable,
	float WorldSizeUU,
	int32 MinRegionPixels,
	bool bUse8Connected,
	bool bSkipOcean)
{
	// --- 1. Editor world -------------------------------------------------------
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethZones: no editor world available."));
		return 0;
	}

	// --- 2. Resolve + read the PNG file ---------------------------------------
	FString Path = BiomePngPath;
	if (Path.IsEmpty())
	{
		Path = FPaths::ProjectDir() / TEXT("MapData/caldreth_biome.png");
	}
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethZones: biome PNG not found: %s"), *Path);
		return 0;
	}

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethZones: failed to read %s"), *Path);
		return 0;
	}

	// --- 3. Decode to 8-bit greyscale -----------------------------------------
	IImageWrapperModule& ImageWrapperModule =
		FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethZones: %s is not a valid PNG."), *Path);
		return 0;
	}

	const int32 Width = Wrapper->GetWidth();
	const int32 Height = Wrapper->GetHeight();
	TArray64<uint8> Gray;
	if (!Wrapper->GetRaw(ERGBFormat::Gray, 8, Gray) || Gray.Num() < (int64)Width * Height)
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethZones: failed to decode %s as 8-bit greyscale."), *Path);
		return 0;
	}

	if (!IsValid(ZoneTable))
	{
		UE_LOG(LogCaldrethImport, Warning,
			TEXT("ImportCaldrethZones: ZoneTable is null — zones will spawn but IsSpawnable/IsHabitable stay false."));
	}

	const uint8 MaxBiomeId = static_cast<uint8>(EZoneType::Oasis); // 11 — legend upper bound

	// --- 4. Flood-fill connected components (iterative, BFS) -------------------
	TBitArray<> Visited(false, Width * Height);
	TArray<int32> Stack;
	TMap<EZoneType, int32> CountPerBiome;
	int32 Placed = 0;
	int32 SkippedNoise = 0;
	int32 SkippedUnknown = 0;

	static const int32 DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
	static const int32 DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
	const int32 NeighbourCount = bUse8Connected ? 8 : 4;

	const FScopedTransaction Transaction(LOCTEXT("ImportZones", "Import Caldreth Zones"));

	for (int32 Start = 0; Start < Width * Height; ++Start)
	{
		if (Visited[Start])
		{
			continue;
		}

		const uint8 BiomeVal = Gray[Start];

		// Collect the whole contiguous region of this biome value.
		Stack.Reset();
		Stack.Push(Start);
		Visited[Start] = true;

		int64 SumCol = 0, SumRow = 0;
		int32 Size = 0;
		int32 MinC = Width, MaxC = 0, MinR = Height, MaxR = 0;

		while (Stack.Num() > 0)
		{
			const int32 P = Stack.Pop();
			const int32 C = P % Width;
			const int32 R = P / Width;

			SumCol += C; SumRow += R; ++Size;
			MinC = FMath::Min(MinC, C); MaxC = FMath::Max(MaxC, C);
			MinR = FMath::Min(MinR, R); MaxR = FMath::Max(MaxR, R);

			for (int32 N = 0; N < NeighbourCount; ++N)
			{
				const int32 NC = C + DX[N];
				const int32 NR = R + DY[N];
				if (NC < 0 || NC >= Width || NR < 0 || NR >= Height)
				{
					continue;
				}
				const int32 NP = NR * Width + NC;
				if (Visited[NP] || Gray[NP] != BiomeVal)
				{
					continue;
				}
				Visited[NP] = true;
				Stack.Push(NP);
			}
		}

		// --- 4a. Region filters ---
		if (Size < MinRegionPixels)
		{
			++SkippedNoise;
			continue;
		}
		if (bSkipOcean && BiomeVal == static_cast<uint8>(EZoneType::Ocean))
		{
			continue;
		}
		if (BiomeVal > MaxBiomeId)
		{
			UE_LOG(LogCaldrethImport, Warning,
				TEXT("ImportCaldrethZones: pixel biome id %u is outside the EZoneType legend (0..%u) — region of %d px skipped."),
				BiomeVal, MaxBiomeId, Size);
			++SkippedUnknown;
			continue;
		}
		const EZoneType Zone = static_cast<EZoneType>(BiomeVal);

		// --- 4b. Normalized centroid (x east = col, y south = row; row 0 = north) ---
		const float CX = (static_cast<float>(SumCol) / Size + 0.5f) / Width;
		const float CY = (static_cast<float>(SumRow) / Size + 0.5f) / Height;

		// Centered on origin so the island straddles (0,0).
		const FVector Location((CX - 0.5f) * WorldSizeUU, (CY - 0.5f) * WorldSizeUU, 0.f);

		ACaldrethZone* ZoneActor = World->SpawnActor<ACaldrethZone>(ACaldrethZone::StaticClass(), FTransform(Location));
		if (!IsValid(ZoneActor))
		{
			UE_LOG(LogCaldrethImport, Warning, TEXT("ImportCaldrethZones: failed to spawn ACaldrethZone for biome %u."), BiomeVal);
			continue;
		}

		ZoneActor->ZoneType = Zone;
		ZoneActor->ZoneTable = ZoneTable;
		ZoneActor->WorldSizeUU = WorldSizeUU;   // so GetZoneAtLocation can map world->normalized later

		// Normalized axis-aligned bbox as a first-pass outline (4 CW corners).
		// A true Moore-boundary trace / spatial index is ETAP 5 territory.
		ZoneActor->NormalizedOutline = {
			FVector2D(static_cast<float>(MinC) / Width,       static_cast<float>(MinR) / Height),
			FVector2D(static_cast<float>(MaxC + 1) / Width,   static_cast<float>(MinR) / Height),
			FVector2D(static_cast<float>(MaxC + 1) / Width,   static_cast<float>(MaxR + 1) / Height),
			FVector2D(static_cast<float>(MinC) / Width,       static_cast<float>(MaxR + 1) / Height)
		};

		ZoneActor->SetActorLabel(FString::Printf(TEXT("Zone_%s_%d"),
			*ACaldrethZone::ZoneTypeToRowName(Zone).ToString(), Placed));

		// Props were set after spawn — re-run construction so OnConstruction caches the FZoneDef.
		ZoneActor->RerunConstructionScripts();

		CountPerBiome.FindOrAdd(Zone) += 1;
		++Placed;
	}

	// --- 5. Report -------------------------------------------------------------
	UE_LOG(LogCaldrethImport, Log,
		TEXT("ImportCaldrethZones: placed %d zones from %s (%dx%d, WorldSize=%.0f uu, %s-connected). Skipped %d noise, %d unknown-biome."),
		Placed, *Path, Width, Height, WorldSizeUU, bUse8Connected ? TEXT("8") : TEXT("4"), SkippedNoise, SkippedUnknown);

	for (const TPair<EZoneType, int32>& Pair : CountPerBiome)
	{
		UE_LOG(LogCaldrethImport, Log, TEXT("  %s: %d zone(s)"),
			*ACaldrethZone::ZoneTypeToRowName(Pair.Key).ToString(), Pair.Value);
	}

	return Placed;
}

int32 UCaldrethImportLibrary::ImportCaldrethPOIs(
	FString ManifestPath,
	float WorldSizeUU)
{
	// --- 1. Editor world -------------------------------------------------------
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethPOIs: no editor world available."));
		return 0;
	}

	// --- 2. Resolve + read the manifest ---------------------------------------
	FString Path = ManifestPath;
	if (Path.IsEmpty())
	{
		Path = FPaths::ProjectDir() / TEXT("MapData/caldreth_manifest.json");
	}
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethPOIs: manifest not found: %s"), *Path);
		return 0;
	}

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethPOIs: failed to read %s"), *Path);
		return 0;
	}

	// --- 3. Parse JSON ---------------------------------------------------------
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethPOIs: %s is not valid JSON."), *Path);
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* Pois = nullptr;
	if (!Root->TryGetArrayField(TEXT("points_of_interest"), Pois) || Pois == nullptr)
	{
		UE_LOG(LogCaldrethImport, Warning,
			TEXT("ImportCaldrethPOIs: no 'points_of_interest' array in %s — nothing placed."), *Path);
		return 0;
	}

	// --- 4. Spawn one marker per POI ------------------------------------------
	const FScopedTransaction Transaction(LOCTEXT("ImportPOIs", "Import Caldreth POIs"));

	int32 Placed = 0;
	int32 Skipped = 0;
	for (const TSharedPtr<FJsonValue>& Val : *Pois)
	{
		const TSharedPtr<FJsonObject> Obj = Val.IsValid() ? Val->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			++Skipped;
			continue;
		}

		double X = 0.0, Y = 0.0;
		if (!Obj->TryGetNumberField(TEXT("x"), X) || !Obj->TryGetNumberField(TEXT("y"), Y))
		{
			UE_LOG(LogCaldrethImport, Warning, TEXT("ImportCaldrethPOIs: a POI is missing x/y — skipped."));
			++Skipped;
			continue;
		}

		FString Name, Kind, Role;
		Obj->TryGetStringField(TEXT("name"), Name);
		Obj->TryGetStringField(TEXT("kind"), Kind);
		Obj->TryGetStringField(TEXT("role"), Role);

		// Same centered mapping as zones: x east (col), y south (row), row 0 = north.
		// Markers therefore land exactly on top of the matching ACaldrethZone centroids.
		const FVector Location(
			(static_cast<float>(X) - 0.5f) * WorldSizeUU,
			(static_cast<float>(Y) - 0.5f) * WorldSizeUU,
			0.f);

		ACaldrethPOIMarker* Marker = World->SpawnActor<ACaldrethPOIMarker>(
			ACaldrethPOIMarker::StaticClass(), FTransform(Location));
		if (!IsValid(Marker))
		{
			UE_LOG(LogCaldrethImport, Warning, TEXT("ImportCaldrethPOIs: failed to spawn marker for '%s'."), *Name);
			++Skipped;
			continue;
		}

		Marker->PoiName = Name;
		Marker->Kind = FName(*Kind);
		Marker->PoiRole = FName(*Role);
		Marker->ApplyRoleTag();   // SpawnActor's OnConstruction ran before PoiRole was set — tag now.

		Marker->SetActorLabel(Name.IsEmpty()
			? FString::Printf(TEXT("POI_%d"), Placed)
			: FString::Printf(TEXT("POI_%s"), *Name));

		UE_LOG(LogCaldrethImport, Log, TEXT("  POI '%s' (kind=%s, role=%s) at (%.0f, %.0f)."),
			*Name, *Kind, *Role, Location.X, Location.Y);

		++Placed;
	}

	// --- 5. Report -------------------------------------------------------------
	UE_LOG(LogCaldrethImport, Log,
		TEXT("ImportCaldrethPOIs: placed %d marker(s) from %s (WorldSize=%.0f uu). Skipped %d."),
		Placed, *Path, WorldSizeUU, Skipped);

	return Placed;
}

AActor* UCaldrethImportLibrary::ImportCaldrethLandscape(
	FString HeightmapR16Path,
	int32 SizeVerts,
	int32 SubsectionSizeQuads,
	int32 NumSubsections,
	float WorldSizeUU,
	float ZScale,
	float ZOffsetUU)
{
	// --- 1. Editor world -------------------------------------------------------
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethLandscape: no editor world available."));
		return nullptr;
	}

	// --- 2. Validate the component layout: (SizeVerts-1) quads must tile evenly ----
	const int32 TotalQuads = SizeVerts - 1;
	const int32 ComponentSizeQuads = NumSubsections * SubsectionSizeQuads;
	if (SizeVerts < 2 || ComponentSizeQuads <= 0 || (TotalQuads % ComponentSizeQuads) != 0)
	{
		UE_LOG(LogCaldrethImport, Error,
			TEXT("ImportCaldrethLandscape: bad layout — (SizeVerts-1)=%d not divisible by NumSubsections*SubsectionSizeQuads=%d."),
			TotalQuads, ComponentSizeQuads);
		return nullptr;
	}
	const int32 ComponentCount = TotalQuads / ComponentSizeQuads;

	// --- 3. Resolve + read the .r16 (little-endian uint16) ---------------------
	FString Path = HeightmapR16Path;
	if (Path.IsEmpty())
	{
		Path = FPaths::ProjectDir() / TEXT("MapData/caldreth_height_505.r16");
	}
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethLandscape: heightmap not found: %s"), *Path);
		return nullptr;
	}

	TArray<uint8> Raw;
	if (!FFileHelper::LoadFileToArray(Raw, *Path))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethLandscape: failed to read %s"), *Path);
		return nullptr;
	}

	const int32 NumVerts = SizeVerts * SizeVerts;
	if (Raw.Num() != NumVerts * 2)
	{
		UE_LOG(LogCaldrethImport, Error,
			TEXT("ImportCaldrethLandscape: %s is %d bytes, expected %d (%dx%d x 2). Wrong SizeVerts?"),
			*Path, Raw.Num(), NumVerts * 2, SizeVerts, SizeVerts);
		return nullptr;
	}

	TArray<uint16> Heights;
	Heights.SetNumUninitialized(NumVerts);
	uint16 MinH = MAX_uint16, MaxH = 0;
	for (int32 i = 0; i < NumVerts; ++i)
	{
		const uint16 H = static_cast<uint16>(Raw[i * 2] | (static_cast<uint16>(Raw[i * 2 + 1]) << 8)); // LE
		Heights[i] = H;
		MinH = FMath::Min(MinH, H);
		MaxH = FMath::Max(MaxH, H);
	}

	// --- 4. Spawn the Landscape actor, centered on origin ----------------------
	const float QuadSizeUU = WorldSizeUU / static_cast<float>(TotalQuads);
	const FVector Location(-WorldSizeUU * 0.5f, -WorldSizeUU * 0.5f, ZOffsetUU);
	const FVector Scale3D(QuadSizeUU, QuadSizeUU, ZScale);
	const FTransform SpawnTM(FQuat::Identity, Location, Scale3D);

	const FScopedTransaction Transaction(LOCTEXT("ImportLandscape", "Import Caldreth Landscape"));

	ALandscape* Landscape = World->SpawnActor<ALandscape>(ALandscape::StaticClass(), SpawnTM);
	if (!IsValid(Landscape))
	{
		UE_LOG(LogCaldrethImport, Error, TEXT("ImportCaldrethLandscape: failed to spawn ALandscape."));
		return nullptr;
	}
	Landscape->SetActorLabel(TEXT("CaldrethLandscape"));

	// --- 5. Import height data (single default layer, no weightmaps) -----------
	const FGuid LandscapeGuid = FGuid::NewGuid();

	TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
	HeightDataPerLayer.Add(FGuid(), MoveTemp(Heights)); // FGuid() == the persistent/default edit layer

	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
	MaterialLayerDataPerLayer.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

	Landscape->Import(
		LandscapeGuid,
		0, 0, TotalQuads, TotalQuads,          // MinX, MinY, MaxX, MaxY (inclusive quad extents)
		NumSubsections, SubsectionSizeQuads,
		HeightDataPerLayer,
		nullptr,
		MaterialLayerDataPerLayer,
		ELandscapeImportAlphamapType::Additive,
		TArrayView<const FLandscapeLayer>());  // no landscape edit-layers (single default layer)

	// --- 6. Finalize -----------------------------------------------------------
	ULandscapeInfo* Info = Landscape->CreateLandscapeInfo();
	if (Info)
	{
		Info->UpdateLayerInfoMap(Landscape);
	}
	Landscape->PostEditChange();

	UE_LOG(LogCaldrethImport, Log,
		TEXT("ImportCaldrethLandscape: %dx%d verts (%d comps of %d quads), from %s. 16-bit range [%u..%u]. ")
		TEXT("Scale=(%.2f,%.2f,%.2f) Loc=(%.0f,%.0f,%.0f). Expected world Z: min~%.0f max~%.0f UU."),
		SizeVerts, SizeVerts, ComponentCount * ComponentCount, ComponentSizeQuads, *Path, MinH, MaxH,
		QuadSizeUU, QuadSizeUU, ZScale, Location.X, Location.Y, Location.Z,
		ZOffsetUU + (static_cast<float>(MinH) - 32768.f) / 128.f * ZScale,
		ZOffsetUU + (static_cast<float>(MaxH) - 32768.f) / 128.f * ZScale);

	return Landscape;
}

#undef LOCTEXT_NAMESPACE
