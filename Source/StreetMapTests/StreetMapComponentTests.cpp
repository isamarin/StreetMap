#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "StreetMap.h"
#include "StreetMapComponent.h"

namespace StreetMapTests
{
	/** Builds a transient UStreetMap asset containing a single road with the given points. */
	UStreetMap* MakeStreetMapWithSingleRoad( UObject* Outer, const TArray<FVector2D>& RoadPoints )
	{
		UStreetMap* Map = NewObject<UStreetMap>( Outer );

		FStreetMapRoad& Road = Map->GetRoads().AddDefaulted_GetRef();
		Road.RoadType = EStreetMapRoadType::Street;
		Road.RoadPoints = RoadPoints;
		Road.NodeIndices.Init( INDEX_NONE, RoadPoints.Num() );

		return Map;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FStreetMapComponentBuildsMeshForValidRoadTest, "StreetMap.Runtime.StreetMapComponent.BuildsMeshForValidRoad", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FStreetMapComponentBuildsMeshForValidRoadTest::RunTest( const FString& Parameters )
{
	// Sanity/control case: a single, non-degenerate road segment should produce exactly one quad (4 vertices,
	// 2 triangles / 6 indices).
	UStreetMap* Map = StreetMapTests::MakeStreetMapWithSingleRoad( GetTransientPackage(), { FVector2D( 0.0, 0.0 ), FVector2D( 1000.0, 0.0 ) } );

	UStreetMapComponent* Component = NewObject<UStreetMapComponent>( GetTransientPackage() );
	Component->SetStreetMap( Map, false, true );

	TestEqual( TEXT( "A single valid segment should produce one quad's worth of vertices" ), Component->GetRawMeshVertices().Num(), 4 );
	TestEqual( TEXT( "A single valid segment should produce one quad's worth of indices" ), Component->GetRawMeshIndices().Num(), 6 );

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST( FStreetMapComponentSkipsDegenerateLineSegmentTest, "StreetMap.Runtime.StreetMapComponent.SkipsDegenerateLineSegment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FStreetMapComponentSkipsDegenerateLineSegmentTest::RunTest( const FString& Parameters )
{
	// A road with two consecutive, identical points (a real pattern in some OSM data) used to produce a
	// zero-area quad with an undefined tangent basis.  The first segment here is degenerate (Start == End);
	// only the second, valid segment should contribute geometry.
	UStreetMap* Map = StreetMapTests::MakeStreetMapWithSingleRoad( GetTransientPackage(),
		{ FVector2D( 0.0, 0.0 ), FVector2D( 0.0, 0.0 ), FVector2D( 1000.0, 0.0 ) } );

	UStreetMapComponent* Component = NewObject<UStreetMapComponent>( GetTransientPackage() );
	Component->SetStreetMap( Map, false, true );

	TestEqual( TEXT( "The degenerate segment should have been skipped, leaving only the valid segment's quad" ), Component->GetRawMeshVertices().Num(), 4 );
	TestEqual( TEXT( "The degenerate segment should have been skipped, leaving only the valid segment's triangles" ), Component->GetRawMeshIndices().Num(), 6 );

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
