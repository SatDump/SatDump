//
// Script JSON.
//
#ifndef _ScriptJSON_h_
#define _ScriptJSON_h_


#ifndef ANGELSCRIPT_H
// Avoid having to inform include path if header is already include before
#include "angelscript/angelscript.h"
#endif

#include <string>
typedef std::string jsonKey_t;

#include "nlohmann/json.hpp"
using json = nlohmann::json;

BEGIN_AS_NAMESPACE

class CScriptArray;

enum CScriptJsonType
{
    OBJECT_VALUE,
    ARRAY_VALUE,
    BOOLEAN_VALUE,
    STRING_VALUE,
    NUMBER_VALUE,
    REAL_VALUE,
    NULL_VALUE
};

class CScriptJson
{
public:
	// Factory functions
	static CScriptJson *Create(asIScriptEngine *engine);
	static CScriptJson *Create(asIScriptEngine *engine, json js);

	// Reference counting
	void AddRef() const;
	void Release() const;

	// Reassign the json
	CScriptJson &operator =(bool other);
	CScriptJson &operator =(asINT64 other);
	CScriptJson &operator =(double other);
	CScriptJson &operator =(const std::string &other);
	CScriptJson &operator =(const CScriptArray &other);
	CScriptJson &operator =(const CScriptJson &other);

	// Sets a key/value pair
	void Set(const jsonKey_t &key, const bool &value);
	void Set(const jsonKey_t &key, const asINT64 &value);
	void Set(const jsonKey_t &key, const double &value);
	void Set(const jsonKey_t &key, const std::string &value);
	void Set(const jsonKey_t &key, const CScriptArray &value);

	// Gets the stored value. Returns false if the value isn't compatible
	bool Get(const jsonKey_t &key, bool &value) const;
	bool Get(const jsonKey_t &key, asINT64 &value) const;
	bool Get(const jsonKey_t &key, double &value) const;
	bool Get(const jsonKey_t &key, std::string &value) const;
	bool Get(const jsonKey_t &key, CScriptArray &value) const;

    bool            GetBool();
    std::string     GetString();
    int             GetNumber();
    double          GetReal();
    CScriptArray*   GetArray();

	// Index accessors. If the json is not const it inserts the value if it doesn't already exist
	// If the json is const then a script exception is set if it doesn't exist and a null pointer is returned
	CScriptJson *operator[](const jsonKey_t &key);
	const CScriptJson *operator[](const jsonKey_t &key) const;

	// Returns true if the key is set
	bool Exists(const jsonKey_t &key) const;

	// Returns true if there are no key/value pairs in the json
	bool IsEmpty() const;

	// Returns the number of key/value pairs in the json
	asUINT GetSize() const;

	// Deletes all keys
	void Clear();

	// Get Value type
	CScriptJsonType Type();

	int GetRefCount();

    json             *js_info = NULL;

private:
	// Since the dictionary uses the asAllocMem and asFreeMem functions to allocate memory
	// the constructors are made protected so that the application cannot allocate it
	// manually in a different way
	CScriptJson(asIScriptEngine *engine);

	// We don't want anyone to call the destructor directly, it should be called through the Release method
    ~CScriptJson();

	// Our properties
	asIScriptEngine *engine;
	mutable int      refCount;
};

// This function will determine the configuration of the engine
// and use one of the two functions below to register the dictionary object
void RegisterScriptJson(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
