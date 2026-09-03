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
			// movementConstant gives the message real content unrelated to schemaVersion,
			// so this test only exercises the "field absent" path, not the empty-message check.
			out << R"({"movementConstant": 5, "trackParts": []})";
		}
		PBLocation location;
		CHECK_NOTHROW(parse_json_to_pb(tmp, &location));
		fs::remove(tmp);
		CHECK_FALSE(location.has_schemaversion());
	}

	// JSON that's valid but doesn't correspond to the target message (wrong shape, or a
	// differently-shaped document with no overlapping field names) still parses with an ok
	// status thanks to ignore_unknown_fields, but leaves the message empty. Every message
	// this utility parses represents a real input, so parse_json_to_pb treats that as a
	// failure rather than handing back a silently-empty result.
	TEST_CASE("parse_json_to_pb throws when the parsed message is empty")
	{
		fs::path tmp = fs::temp_directory_path() / "schema_version_test_empty_message.json";
		{
			std::ofstream out(tmp);
			out << R"({"someFieldThatDoesNotExistOnScenario": 1})";
		}
		PBScenario scenario;
		CHECK_THROWS_AS(parse_json_to_pb(tmp, &scenario), std::runtime_error);
		fs::remove(tmp);
	}

	// A message can be non-empty overall (parse_json_to_pb's check above is satisfied) while
	// still missing the one field that actually makes it usable - require_essential_content
	// exists for exactly that: a caller checking the specific content it knows it needs.
	TEST_CASE("require_essential_content: throws when a Location has no trackParts")
	{
		fs::path tmp = fs::temp_directory_path() / "require_essential_content_test_location.json";
		{
			std::ofstream out(tmp);
			out << R"({"movementConstant": 5})";
		}
		PBLocation location;
		parse_json_to_pb(tmp, &location);
		fs::remove(tmp);
		CHECK_THROWS_AS(require_essential_content(tmp, location.trackparts_size() > 0, "trackParts"), std::invalid_argument);
	}

	TEST_CASE("require_essential_content: throws when a Scenario has no trains")
	{
		fs::path tmp = fs::temp_directory_path() / "require_essential_content_test_scenario.json";
		{
			std::ofstream out(tmp);
			out << R"({"startTime": 100})";
		}
		PB_HIP_Scenario scenario;
		parse_json_to_pb(tmp, &scenario);
		fs::remove(tmp);
		bool hasTrains = scenario.in_size() > 0 || scenario.out_size() > 0 ||
			scenario.instanding_size() > 0 || scenario.outstanding_size() > 0;
		CHECK_THROWS_AS(require_essential_content(tmp, hasTrains, "arriving, departing, or standing trains"), std::invalid_argument);
	}

	TEST_CASE("require_essential_content: does not throw when the essential content is present")
	{
		PBLocation location;
		location.add_trackparts();
		CHECK_NOTHROW(require_essential_content("test.json", location.trackparts_size() > 0, "trackParts"));
	}
}
