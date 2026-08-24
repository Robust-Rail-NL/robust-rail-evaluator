#include "doctest/doctest.h"
#include "Utils.h"

#include <fstream>
#include <functional>
#include <sstream>

namespace cTORSTest
{
	// warn_on_schema_version_mismatch() writes synchronously to std::cerr
	// (unlike the async console logger used on the HIP/C# side), so
	// capturing its output here is reliable rather than flaky.
	static std::string CaptureStderr(const std::function<void()>& action)
	{
		std::ostringstream buffer;
		std::streambuf* old = std::cerr.rdbuf(buffer.rdbuf());
		action();
		std::cerr.rdbuf(old);
		return buffer.str();
	}

	TEST_CASE("schemaVersion: matching value produces no warning")
	{
		PBLocation location;
		location.set_schemaversion(EXPECTED_SCHEMA_VERSION);
		std::string output = CaptureStderr(
			[&]() { warn_on_schema_version_mismatch("test.json", location); }
		);
		CHECK(output == "");
	}

	TEST_CASE("schemaVersion: missing warns and does not throw")
	{
		PBLocation location; // schemaVersion left unset
		std::string output;
		CHECK_NOTHROW(
			output = CaptureStderr(
				[&]() { warn_on_schema_version_mismatch("test.json", location); }
			)
		);
		CHECK(output.find("schemaVersion is missing") != std::string::npos);
	}

	TEST_CASE("schemaVersion: mismatched value warns and does not throw")
	{
		PBScenario scenario;
		scenario.set_schemaversion(999);
		std::string output;
		CHECK_NOTHROW(
			output = CaptureStderr(
				[&]() { warn_on_schema_version_mismatch("test.json", scenario); }
			)
		);
		CHECK(output.find("does not match expected") != std::string::npos);
	}

	TEST_CASE("schemaVersion: message types without the field are skipped silently")
	{
		PBRun run; // Run is TORS-internal and has no schemaVersion field
		std::string output = CaptureStderr(
			[&]() { warn_on_schema_version_mismatch("test.json", run); }
		);
		CHECK(output == "");
	}

	TEST_CASE("schemaVersion: parse_json_to_pb reads a present value")
	{
		fs::path tmp = fs::temp_directory_path() / "schema_version_test_location.json";
		{
			std::ofstream out(tmp);
			out << R"({"schemaVersion": 1, "trackParts": []})";
		}
		PBLocation location;
		parse_json_to_pb(tmp, &location);
		fs::remove(tmp);
		CHECK(location.has_schemaversion());
		CHECK(location.schemaversion() == 1);
	}

	TEST_CASE("schemaVersion: parse_json_to_pb does not throw when the field is absent")
	{
		fs::path tmp = fs::temp_directory_path() / "schema_version_test_location_missing.json";
		{
			std::ofstream out(tmp);
			out << R"({"trackParts": []})";
		}
		PBLocation location;
		CHECK_NOTHROW(parse_json_to_pb(tmp, &location));
		fs::remove(tmp);
		CHECK_FALSE(location.has_schemaversion());
	}
}
