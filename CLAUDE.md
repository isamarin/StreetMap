# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

StreetMap is an Unreal Engine plugin (currently targeting UE 5.1+) that imports OpenStreetMap `.osm` XML files into a `UStreetMap` data asset and renders them with a custom mesh component. It is not a standalone buildable project — it's a plugin that lives under a host UE project's `Plugins/StreetMap/` folder.

## Build / test / run

There is no CLI build, lint, or test suite in this repo — it's compiled as part of a host Unreal Engine project via the UE Build Tool (UBT).

- To build: drop/symlink this repo into `<HostProject>/Plugins/StreetMap/`, then regenerate project files and build the host project (Visual Studio on Windows, Xcode on Mac) or run UBT directly, e.g. `RunUAT`/`Build.bat`/`Build.sh` against the host `.uproject`.
- There are no automated tests in the repo. Verify changes by loading the host project's editor, importing an `.osm` file, and checking the generated `StreetMapActor` in the viewport.
- Compile errors/warnings are only visible through the host project's build (Visual Studio error list, or UBT console output).

## Module architecture

The plugin is split into two UE modules, declared in [StreetMap.uplugin](StreetMap.uplugin):

- **StreetMapRuntime** (`Source/StreetMapRuntime`, Runtime module) — ships in packaged games. Depends only on `Core`, `CoreUObject`, `Engine`, `RHI`, `RenderCore`, `NavigationSystem`. Contains:
  - `UStreetMap` ([StreetMap.h](Source/StreetMapRuntime/Public/StreetMap.h)) — the serialized data asset: `FStreetMapRoad`, `FStreetMapNode`, `FStreetMapBuilding` structs, plus inline pathfinding helper functions on roads/nodes (connectivity, distance-along-road, cost estimation). This is pure data + query logic, no rendering.
  - `UStreetMapComponent` ([StreetMapComponent.h](Source/StreetMapRuntime/Public/StreetMapComponent.h)/.cpp) — a custom `UMeshComponent` (not a `StaticMeshComponent`) that procedurally builds a renderable mesh from a `UStreetMap` asset's raw road/building data at load time (`GenerateMesh()`). Mesh-generation tunables (thickness/color per road type, 3D building extrusion, lit vs. unlit buildings) live in `FStreetMapMeshBuildSettings` at the top of [StreetMap.h](Source/StreetMapRuntime/Public/StreetMap.h).
  - `FStreetMapSceneProxy` ([StreetMapSceneProxy.h](Source/StreetMapRuntime/StreetMapSceneProxy.h)) — the render-thread scene proxy consuming the cached `FStreetMapVertex`/index arrays; draws everything in a single draw call (quad-strip roads, no tessellation, no UVs yet).
  - `AStreetMapActor` ([StreetMapActor.h](Source/StreetMapRuntime/Public/StreetMapActor.h)) — trivial actor wrapping a `UStreetMapComponent`.

- **StreetMapImporting** (`Source/StreetMapImporting`, Editor-only module) — editor/import-time only, additionally depends on `UnrealEd`, `XmlParser`, `AssetTools`, `Slate`/`SlateCore`/`EditorStyle`, `PropertyEditor`, `RawMesh`, `AssetRegistry`, and on `StreetMapRuntime`. Contains:
  - `FOSMFile` ([OSMFile.h](Source/StreetMapImporting/OSMFile.h)/.cpp) — a `FastXml`-based OSM XML parser. Builds an in-memory graph of `FOSMNodeInfo`/`FOSMWayInfo` in double-precision geographic coordinates, close to the raw OSM structure. `EOSMWayType` enumerates all recognized `highway=*`/`building=*` tag values.
  - `UStreetMapFactory` ([StreetMapFactory.h](Source/StreetMapImporting/StreetMapFactory.h)/.cpp) — the `UFactory` that turns an `.osm` file into a `UStreetMap` asset: projects lat/long to a flat 2D plane relative to the map's average lat/long (Sanson-Flamsteed/sinusoidal projection), converts meters to UE centimeters, classifies ways into `EStreetMapRoadType` (Street/MajorRoad/Highway/Other), builds `FStreetMapRoad`/`FStreetMapBuilding` entries, and collapses OSM nodes down to only the ones needed for connectivity (intersections + road endpoints) to save memory.
  - `UStreetMapReimportFactory` — supports drag-reimport of an existing `UStreetMap` asset from its original `.osm` source path (via `AssetImportData`).
  - `UStreetMapActorFactory` — lets you drag a `UStreetMap` asset into the viewport to spawn an `AStreetMapActor`.
  - `StreetMapComponentDetails` / `StreetMapStyle` / `StreetMapAssetTypeActions` — editor UI glue (details panel customization, asset icons, content browser integration).

### Import → runtime data flow

`.osm` XML → `FOSMFile` (double-precision, near-raw OSM graph) → `UStreetMapFactory::LoadFromOpenStreetMapXMLFile` (projects + scales + filters into `UStreetMap`'s single-precision `Roads`/`Nodes`/`Buildings` arrays, serialized to disk) → `UStreetMapComponent::GenerateMesh()` (builds `FStreetMapVertex`/index buffers at component load/edit time, not at import time) → `FStreetMapSceneProxy` (renders on the render thread).

### Coordinate/precision notes worth knowing before touching import or data code

- OSM lat/long is projected to a flat 2D plane at import time (UE has no native spherical/geographic coordinate support); precision is truncated from double to single float when writing the `UStreetMap` asset. Large maps or high-precision use cases (e.g. real GPS navigation) would need to change this.
- Node filtering: only OSM nodes that are road intersections or road endpoints are kept as `FStreetMapNode`s; interior points along a road are stored only as raw `RoadPoints` with `INDEX_NONE` in `NodeIndices`. Don't assume every point on a road has a corresponding node.
- `FStreetMapRoad::GetRoadIndex`/`FStreetMapNode::GetNodeIndex` use pointer arithmetic against the owning `UStreetMap`'s array (`this - Array.GetData()`), so these structs must only ever be referenced/passed around by index or by reference into their owning array — copying them elsewhere breaks index computation.

## Known limitations (see README "Known Issues" for full list)

- Files >2GB fail to import (UE limitation).
- Some non-standard OSM XML (e.g. single-quoted attribute values) doesn't parse.
- Blueprint scripting support is minimal; most APIs are C++-oriented and many methods are inlined for performance.
- Pathfinding helper functions exist on `FStreetMapNode`/`FStreetMapRoad` but no example pathfinding algorithm ships with the plugin.
