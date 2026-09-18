#include "Board/LDBoardGeometry.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Board/LDViewTransform.h"
#include "Data/LDGameData.h"
#include "Misc/AutomationTest.h"

namespace
{
	FLDGameRules GeometryFixture()
	{
		FLDGameRules Rules;
		Rules.Columns = 6;
		Rules.Rows = 3;
		Rules.CellsPerPlayer = 18;
		Rules.CellSizeCm = 140;
		Rules.XCentersCm = {-350, -210, -70, 70, 210, 350};
		Rules.YCentersByPlayer = {{-420, -280, -140}, {140, 280, 420}};
		return Rules;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0CellOwnershipTest, "LD.P0.G1.Board.CanonicalCellsAndOwnership",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0CellOwnershipTest::RunTest(const FString& Parameters)
{
	FLDBoardGeometry Geometry;
	FString Error;
	TestTrue(TEXT("Confirmed 6x3 geometry initializes"), Geometry.Initialize(GeometryFixture(), Error));
	for (int32 Cell = 0; Cell < 36; ++Cell)
	{
		FVector Center;
		TestTrue(TEXT("All 36 logical cells exist"), Geometry.TryGetCellCenter(Cell, Center));
		const int32 Owner = Cell / 18;
		const int32 Row = (Cell % 18) / 6;
		const int32 Column = Cell % 6;
		TestEqual(TEXT("CellId encodes canonical X column"), Center.X, -350.0 + 140 * Column);
		TestEqual(TEXT("CellId encodes canonical Y row"), Center.Y, (Owner == 0 ? -420.0 : 140.0) + 140 * Row);
		TestEqual(TEXT("Owner may select every personal cell"), Geometry.ValidateSelection(Owner, Cell),
		               ELDCellInputResult::Selected);
		TestEqual(TEXT("Other owner is rejected for every cell"), Geometry.ValidateSelection(1 - Owner, Cell),
		               ELDCellInputResult::NotOwner);
		for (double XOffset : {-62.0, 0.0, 62.0})
		{
			for (double YOffset : {-62.0, 0.0, 62.0})
			{
				int32 Hit = INDEX_NONE;
				TestTrue(TEXT("Visible cell interiors have no input holes"),
				              Geometry.TryGetCellAtCanonicalPosition(Center + FVector(XOffset, YOffset, 0), Hit));
				TestEqual(TEXT("Cell interior resolves to the same logical identity"), Hit, Cell);
			}
		}
	}
	int32 Hit = INDEX_NONE;
	TestFalse(TEXT("Shared monster lane is not a placement cell"),
	               Geometry.TryGetCellAtCanonicalPosition(FVector::ZeroVector, Hit));
	TestEqual(TEXT("Invalid cell cannot be selected"), Geometry.ValidateSelection(0, 36),
	               ELDCellInputResult::OutsideBoard);
	TestTrue(TEXT("Shared edge has one deterministic owner cell"),
	              Geometry.TryGetCellAtCanonicalPosition(FVector(-280, -420, 0), Hit));
	TestEqual(TEXT("Half-open right edge belongs to the next column"), Hit, 1);
	FLDGameRules Invalid = GeometryFixture();
	Invalid.Rows = 4;
	TestFalse(TEXT("Unexpected shape fails explicitly"), Geometry.Initialize(Invalid, Error));
	TestTrue(TEXT("Invalid replacement preserves previously validated geometry"), Geometry.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ViewTransformTest, "LD.P0.G1.Board.LocalReflectionContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ViewTransformTest::RunTest(const FString& Parameters)
{
	for (int32 Player = 0; Player < 2; ++Player)
	{
		const FVector Spawn(490, Player == 0 ? -560 : 560, 0);
		const FVector Presented = FLDViewTransform::ToPresentation(Spawn, Player);
		TestEqual(TEXT("Both local entrances are on the left under yaw90 camera"), Presented.X, 490.0);
		TestEqual(TEXT("Both local entrances belong to the lower board"), Presented.Y, -560.0);
		TestEqual(TEXT("Input inverse recovers unchanged canonical location"),
		               FLDViewTransform::ToCanonical(Presented, Player), Spawn);
		const FVector CentralStart = FLDViewTransform::ToPresentation(FVector(490, 0, 0), Player);
		const FVector CentralEnd = FLDViewTransform::ToPresentation(FVector(-490, 0, 0), Player);
		TestTrue(TEXT("Shared lane moves in the same screen-right direction"), CentralEnd.X < CentralStart.X);
		TestEqual(TEXT("Shared lane does not move vertically between views"), CentralStart.Y, 0.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ViewportFitTest, "LD.P0.G1.Board.ViewportAndSafeAreaFit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ViewportFitTest::RunTest(const FString& Parameters)
{
	FLDBoardViewportLayout Reference;
	TestTrue(TEXT("Reference v2 viewport fits"),
	              Reference.Initialize(FVector2D(1080, 2340), FBox2D(FVector2D::ZeroVector, FVector2D(1080, 2340)),
	                                   FVector2D(1120, 1260)));
	TestTrue(TEXT("Reference field starts at UI (60,520)"),
	              Reference.FieldRectPixels.Min.Equals(FVector2D(60, 520), 0.001));
	TestTrue(TEXT("Reference field ends at UI (1020,1600)"),
	              Reference.FieldRectPixels.Max.Equals(FVector2D(1020, 1600), 0.001));
	TestTrue(TEXT("140cm cell projects to 120px"), FMath::IsNearlyEqual(Reference.PixelsPerCm * 140, 120.0, 0.001));
	const FVector2D Sizes[] = {FVector2D(540, 1170), FVector2D(720, 1280), FVector2D(800, 1280), FVector2D(1280, 720),
	                           FVector2D(1200, 1600)};
	for (const FVector2D& Size : Sizes)
	{
		for (bool bInset : {false, true})
		{
			const FBox2D Safe =
			    bInset ? FBox2D(FVector2D(20, 44), Size - FVector2D(12, 32)) : FBox2D(FVector2D::ZeroVector, Size);
			FLDBoardViewportLayout Layout;
			TestTrue(TEXT("Actual aspect or asymmetric safe area fits"),
			              Layout.Initialize(Size, Safe, FVector2D(1120, 1260)));
			const FVector2D Field = Layout.FieldRectPixels.GetSize();
			TestTrue(TEXT("Field keeps 8:9 aspect, including landscape"),
			              FMath::IsNearlyEqual(Field.X / Field.Y, 8.0 / 9.0, 0.0001));
			TestTrue(TEXT("All field bounds fit the safe rectangle"),
			              Layout.FieldRectPixels.Min.X >= Safe.Min.X && Layout.FieldRectPixels.Min.Y >= Safe.Min.Y &&
			                  Layout.FieldRectPixels.Max.X <= Safe.Max.X && Layout.FieldRectPixels.Max.Y <= Safe.Max.Y);
			TestTrue(TEXT("Camera pixel density equals field fit density"),
			              FMath::IsNearlyEqual(Size.X / Layout.OrthoWidthCm, Layout.PixelsPerCm, 0.0001));
		}
	}
	TestFalse(TEXT("A minimized viewport must not create an invalid camera"),
	               Reference.Initialize(FVector2D::ZeroVector, FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector),
	                                    FVector2D(1120, 1260)));
	return true;
}

#endif
