#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "OSMFile.h"

namespace StreetMapTests
{
	/** Parses an in-memory OSM XML buffer with FOSMFile, the same way the importer does for .osm files. */
	bool ParseOSMBuffer( FOSMFile& OSMFile, const FString& XmlText )
	{
		FString MutableBuffer = XmlText;
		return OSMFile.LoadOpenStreetMapFile( MutableBuffer, /* bIsFilePathActuallyTextBuffer */ true, /* FeedbackContext */ nullptr );
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FOSMFileParsesBasicWaysTest, "StreetMap.Importing.OSMFile.ParsesBasicWays", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FOSMFileParsesBasicWaysTest::RunTest( const FString& Parameters )
{
	const FString OSMXml = TEXT(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?><osm version=\"0.6\">"
		"<node id=\"1\" lat=\"51.500\" lon=\"-0.100\"/>"
		"<node id=\"2\" lat=\"51.501\" lon=\"-0.101\"/>"
		"<node id=\"3\" lat=\"51.502\" lon=\"-0.102\"/>"
		"<node id=\"4\" lat=\"51.503\" lon=\"-0.103\"/>"
		"<way id=\"100\"><nd ref=\"1\"/><nd ref=\"2\"/><tag k=\"highway\" v=\"residential\"/><tag k=\"name\" v=\"Test Street\"/></way>"
		"<way id=\"200\"><nd ref=\"1\"/><nd ref=\"3\"/><nd ref=\"4\"/><nd ref=\"1\"/><tag k=\"building\" v=\"yes\"/></way>"
		"</osm>" );

	FOSMFile OSMFile;
	if( !TestTrue( TEXT( "Parsing a well-formed file should succeed" ), StreetMapTests::ParseOSMBuffer( OSMFile, OSMXml ) ) )
	{
		return false;
	}

	TestEqual( TEXT( "Two ways should have been parsed" ), OSMFile.Ways.Num(), 2 );
	TestEqual( TEXT( "Four nodes should have been parsed" ), OSMFile.NodeMap.Num(), 4 );

	if( OSMFile.Ways.Num() == 2 )
	{
		TestEqual( TEXT( "First way should be a residential road" ), (int32)OSMFile.Ways[ 0 ]->WayType, (int32)FOSMFile::EOSMWayType::Residential );
		TestEqual( TEXT( "First way's name should be parsed" ), OSMFile.Ways[ 0 ]->Name, FString( TEXT( "Test Street" ) ) );
		TestEqual( TEXT( "Second way should be a building" ), (int32)OSMFile.Ways[ 1 ]->WayType, (int32)FOSMFile::EOSMWayType::Building );
		TestEqual( TEXT( "Second way should have 4 node refs (closed polygon)" ), OSMFile.Ways[ 1 ]->Nodes.Num(), 4 );
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST( FOSMFileSkipsUndeclaredNodeRefTest, "StreetMap.Importing.OSMFile.SkipsUndeclaredNodeRef", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FOSMFileSkipsUndeclaredNodeRefTest::RunTest( const FString& Parameters )
{
	// Way 100 references node id 2, but only node id 1 is ever declared -- this mirrors a bounding-box-clipped
	// OSM export.  Before the fix, resolving the undeclared ref returned null and was dereferenced unconditionally,
	// crashing the importer.
	const FString OSMXml = TEXT(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?><osm version=\"0.6\">"
		"<node id=\"1\" lat=\"51.5\" lon=\"-0.1\"/>"
		"<way id=\"100\"><nd ref=\"1\"/><nd ref=\"2\"/><tag k=\"highway\" v=\"residential\"/></way>"
		"</osm>" );

	FOSMFile OSMFile;
	if( !TestTrue( TEXT( "Parsing should succeed even with an undeclared node reference" ), StreetMapTests::ParseOSMBuffer( OSMFile, OSMXml ) ) )
	{
		return false;
	}

	TestEqual( TEXT( "One way should have been parsed" ), OSMFile.Ways.Num(), 1 );
	if( OSMFile.Ways.Num() == 1 )
	{
		TestEqual( TEXT( "The undeclared node ref should have been skipped, leaving only the declared node" ), OSMFile.Ways[ 0 ]->Nodes.Num(), 1 );
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST( FOSMFileHandlesDuplicateNodeIdTest, "StreetMap.Importing.OSMFile.HandlesDuplicateNodeId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FOSMFileHandlesDuplicateNodeIdTest::RunTest( const FString& Parameters )
{
	// Two <node> elements share the same id.  Before the fix, the first FOSMNodeInfo leaked silently when
	// overwritten in the map.  Verify parsing still succeeds and exactly one node survives, with the second
	// declaration's coordinates (since nothing had referenced the first one yet).
	const FString OSMXml = TEXT(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?><osm version=\"0.6\">"
		"<node id=\"1\" lat=\"10.0\" lon=\"20.0\"/>"
		"<node id=\"1\" lat=\"30.0\" lon=\"40.0\"/>"
		"</osm>" );

	FOSMFile OSMFile;
	if( !TestTrue( TEXT( "Parsing should succeed with a duplicate node id" ), StreetMapTests::ParseOSMBuffer( OSMFile, OSMXml ) ) )
	{
		return false;
	}

	TestEqual( TEXT( "Only one node should remain in the map for the duplicate id" ), OSMFile.NodeMap.Num(), 1 );

	FOSMFile::FOSMNodeInfo* const* FoundNode = OSMFile.NodeMap.Find( 1 );
	if( TestNotNull( TEXT( "Node id 1 should be present in the map" ), FoundNode ) )
	{
		TestEqual( TEXT( "The surviving node should have the second declaration's latitude" ), (*FoundNode)->Latitude, 30.0 );
		TestEqual( TEXT( "The surviving node should have the second declaration's longitude" ), (*FoundNode)->Longitude, 40.0 );
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST( FOSMFileIgnoresTagValueWithoutKeyTest, "StreetMap.Importing.OSMFile.IgnoresTagValueWithoutKey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter )

bool FOSMFileIgnoresTagValueWithoutKeyTest::RunTest( const FString& Parameters )
{
	// The first <tag> in the file has its "v" attribute before "k" (not guaranteed by XML attribute order, but
	// seen from some non-standard exporters).  This used to null-dereference CurrentWayTagKey.  Verify it's now
	// ignored, and that a later, well-formed tag on the same way is still applied correctly.
	const FString OSMXml = TEXT(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?><osm version=\"0.6\">"
		"<node id=\"1\" lat=\"0.0\" lon=\"0.0\"/><node id=\"2\" lat=\"0.001\" lon=\"0.001\"/>"
		"<way id=\"100\"><nd ref=\"1\"/><nd ref=\"2\"/>"
		"<tag v=\"MainStreet\" k=\"name\"/>"
		"<tag k=\"highway\" v=\"residential\"/>"
		"</way></osm>" );

	FOSMFile OSMFile;
	if( !TestTrue( TEXT( "Parsing should succeed despite the out-of-order tag attribute" ), StreetMapTests::ParseOSMBuffer( OSMFile, OSMXml ) ) )
	{
		return false;
	}

	TestEqual( TEXT( "One way should have been parsed" ), OSMFile.Ways.Num(), 1 );
	if( OSMFile.Ways.Num() == 1 )
	{
		TestEqual( TEXT( "The malformed tag's value should have been ignored, leaving the way's name empty" ), OSMFile.Ways[ 0 ]->Name, FString() );
		TestEqual( TEXT( "The well-formed highway tag after the malformed one should still be applied" ), (int32)OSMFile.Ways[ 0 ]->WayType, (int32)FOSMFile::EOSMWayType::Residential );
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
