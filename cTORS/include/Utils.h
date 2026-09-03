/** \file Utils.h
 * Defines common useful methods
 */
#pragma once
#ifndef UTILS_H
#define UTILS_H
//!\cond SYS_HEADER
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <map>
#include <queue>
//!\endcond
#include "Proto.h"
namespace fs = std::filesystem;
using namespace std;

//!\cond NO_DOC
#ifndef DEBUG
#define DEBUG 1
#endif // !DEBUG
// Tracing is separate from assertions: an assertion-enabled build for integration
// testing wants the checks without ~9000 lines of trace per run drowning the
// result files. Defaults to DEBUG, so a plain debug build is unaffected.
#ifndef DEBUG_OUTPUT
#define DEBUG_OUTPUT DEBUG
#endif // !DEBUG_OUTPUT
#ifndef debug_out
#define debug_out(s) \
if(DEBUG_OUTPUT) { std::cout << s << std::endl; }
#endif

#ifndef DELETE_LIST
#define DELETE_LIST(l) l.remove_if([](auto e) { delete e; return true; });
#endif

#ifndef DELETE_VECTOR
#define DELETE_VECTOR(l) l.erase(remove_if(l.begin(), l.end(), [](auto e) {delete e; return true;}), l.end());
#endif

template<typename Base, typename T>
inline bool instanceof(const T* ptr) {
    return dynamic_cast<const Base*>(ptr) != nullptr;
}

#ifndef STREAM_OPERATOR
#define STREAM_OPERATOR(name) \
inline ostream& operator<<(ostream& str, const name& v) { \
	str << v.toString(); \
    return str; \
} \
inline ostream& operator<<(ostream& str, const name* v) { \
	str << v->toString(); \
    return str; \
}
#endif

template<typename T>
vector<const T*> copy_of(const vector<const T*>& objs) {
    vector<const T*> res(objs.size());
    for(size_t i=0; i<objs.size(); i++)
        res[i] = new T(*objs[i]);
    return res;
}

template <class It>
string Join(It begin, const It end, const string& sep) {
  ostringstream stream;
  if(begin!=end)
    stream << *begin++;
  while(begin!=end) {
    stream << sep;
    stream << *begin++;
  }
  return stream.str();
}

template <class Obj>
string Join(vector<Obj> objects, const string& sep) {
  return Join(objects.begin(), objects.end(), sep);
}

template <class Obj>
string Join(list<Obj> objects, const string& sep) {
  return Join(objects.begin(), objects.end(), sep);
}

// The shared interchange schemaVersion carried by Location, Scenario, and
// Plan. Independent monotonic integer, decoupled from tool release versions;
// increments only on breaking changes to the wire format. See
// SCHEMA_CHANGELOG.md in robust-rail-generator for what changed at each
// version.
constexpr int EXPECTED_SCHEMA_VERSION = 1;

// Warn-and-continue: a missing or unexpected schemaVersion is logged, never
// a hard reject. Messages that don't declare a schemaVersion field (e.g.
// TORS-internal formats) are silently skipped.
inline void warn_on_schema_version_mismatch(const fs::path& file_path, const google::protobuf::Message& message) {
    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
    const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName("schemaVersion");
    if (field == nullptr)
        return;
    const google::protobuf::Reflection* reflection = message.GetReflection();
    if (!reflection->HasField(message, field)) {
        cerr << "Warning: " << file_path.string() << ": schemaVersion is missing; assuming "
             << EXPECTED_SCHEMA_VERSION << "." << endl;
        return;
    }
    int version = reflection->GetInt32(message, field);
    if (version != EXPECTED_SCHEMA_VERSION) {
        cerr << "Warning: " << file_path.string() << ": schemaVersion " << version
             << " does not match expected " << EXPECTED_SCHEMA_VERSION << "." << endl;
    }
}

inline void parse_json_to_pb(const fs::path& file_path, google::protobuf::Message* message) {
    ifstream fileInput;
    fileInput.open(file_path);
    if (!fileInput.good())
        throw runtime_error("The file " + file_path.string() + " does not exist.");
    stringstream buffer;
    buffer << fileInput.rdbuf();
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    options.case_insensitive_enum_parsing = true;
    auto status = google::protobuf::util::JsonStringToMessage(buffer.str(), message, options);
    debug_out("Parse JSON " << file_path.string() << " / Status: " << status.ToString());
    fileInput.close();
    if (!status.ok())
        throw runtime_error("Failed to parse " + file_path.string() + ": " + status.ToString());
    // A syntactically valid JSON document that doesn't correspond to this message (wrong
    // shape, or a differently-shaped document with no overlapping field names) still parses
    // with an ok status above thanks to ignore_unknown_fields, but leaves the message empty.
    // Every message this utility parses represents a real, non-trivial input (a location,
    // scenario, plan or run), so an all-default result always means the JSON didn't actually
    // match - never a legitimate "empty on purpose" case.
    if (!message->IsInitialized() || message->ByteSizeLong() == 0)
        throw runtime_error("Parsed " + file_path.string() + " into an empty " +
            message->GetDescriptor()->name() + " message; the JSON is likely missing or does not match the expected schema.");
    warn_on_schema_version_mismatch(file_path, *message);
}

inline void parse_json_to_pb(const string& filename, google::protobuf::Message* message) {
    parse_json_to_pb(fs::path(filename), message);
}

inline void parse_pb_to_json(const fs::path& file_path, const google::protobuf::Message& message) {
    ofstream out (file_path);
    if(!out.is_open())
        throw runtime_error("The file " + file_path.string() + " could not be opened.");
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    string output;
    google::protobuf::util::MessageToJsonString(message, &output, options);
    out << output;
    out.close();
}

inline void parse_pb_to_json(const string& filename, const google::protobuf::Message& message) {
    parse_pb_to_json(fs::path(filename), message);
}


template<class T>
inline void hash_combine(size_t& s, const T& v)
{
    hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <class T>
struct hash<pair<T, T>> {
    size_t operator() (const pair<T, T> &pair) const {
        auto x = hash<T>{}(pair.first);
        hash_combine(x, pair.second);
        return x;
    }
};
//!\endcond

#endif