/* stemmer_builder.cc: Builder for stemming algorithms
 *
 * ----START-LICENCE----
 * Copyright 1999,2000 BrightStation PLC
 * 
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#include "config.h"
#include <string>

#include <om/omstem.h>
#include "utils.h"
#include "omlocks.h"

#include "dutch/stem_dutch.h"
#include "english/stem_english.h"
#include "french/stem_french.h"
#include "german/stem_german.h"
#include "italian/stem_italian.h"
#include "portuguese/stem_portuguese.h"
#include "spanish/stem_spanish.h"
#include "porter/stem_porter.h"

////////////////////////////////////////////////////////////

/** The available languages for the stemming algorithms to use. */
enum stemmer_language {
    STEMLANG_NULL,
    STEMLANG_DUTCH,
    STEMLANG_ENGLISH,
    STEMLANG_FRENCH,
    STEMLANG_GERMAN,
    STEMLANG_ITALIAN,
    STEMLANG_PORTUGUESE,
    STEMLANG_SPANISH,
    STEMLANG_PORTER
};

/** The mapping from language strings to language codes. */
stringToType<stemmer_language>
stringToTypeMap<stemmer_language>::types[] = {
    {"de",		STEMLANG_GERMAN},
    {"dutch",		STEMLANG_DUTCH},
    {"en",		STEMLANG_ENGLISH},
    {"english",		STEMLANG_ENGLISH},
    {"es",		STEMLANG_SPANISH},
    {"fr",		STEMLANG_FRENCH},
    {"french",		STEMLANG_FRENCH},
    {"german",		STEMLANG_GERMAN},
    {"it",		STEMLANG_ITALIAN},
    {"italian",		STEMLANG_ITALIAN},
    {"nl",		STEMLANG_DUTCH},
    {"porter",		STEMLANG_PORTER},
    {"portuguese",	STEMLANG_PORTUGUESE},
    {"pt",		STEMLANG_PORTUGUESE},
    {"spanish",		STEMLANG_SPANISH},
    {"",		STEMLANG_NULL}
};

////////////////////////////////////////////////////////////
// OmStemInternal class
// ====================
// Implementation of the OmStem interface

class OmStem::Internal {
    private:
	/** Function pointer to setup the stemmer.
	 */
	void * (* stemmer_setup)();

	/** Function pointer to stem a word.
	 */
	const char * (* stemmer_stem)(void *, const char *, int, int);

	/** Function pointer to close down the stemmer.
	 */
	void (* stemmer_closedown)(void *);

	/** Data used by the stemming algorithm.
	 */
	void * stemmer_data;
	
        /** Return a Stemmer object pointer given a language type.
	 */
        void set_language(stemmer_language lang);
        
	/** Return a stemmer_language enum value from a language
	 *  string.
	 */
	stemmer_language get_stemtype(string language);
    public:
    	/** Initialise the state based on the specified language.
	 */
	Internal(string language);

	~Internal();

	/** Protection against concurrent access.
	 */
	OmLock mutex;

	/** Stem the given word.
	 */
	string stem_word(string word) const;
};

OmStem::Internal::Internal(string language)
	: stemmer_data(0)
{
    stemmer_language lang = get_stemtype(language);
    if (lang == STEMLANG_NULL) {
        // FIXME: use a separate InvalidLanguage exception?
        throw OmInvalidArgumentError("Unknown language specified");
    }
    set_language(lang);
}

OmStem::Internal::~Internal()
{
    if(stemmer_data != 0) {
	stemmer_closedown(stemmer_data);
    }
}

void
OmStem::Internal::set_language(stemmer_language lang)
{
    if(stemmer_data != 0) {
	stemmer_closedown(stemmer_data);
    }
    stemmer_setup = 0;
    switch(lang) {
	case STEMLANG_DUTCH:
	    stemmer_setup = setup_dutch_stemmer;
	    stemmer_stem = dutch_stem;
	    stemmer_closedown = closedown_dutch_stemmer;
	    break;
	case STEMLANG_ENGLISH:
	    stemmer_setup = setup_english_stemmer;
	    stemmer_stem = english_stem;
	    stemmer_closedown = closedown_english_stemmer;
	    break;
	case STEMLANG_FRENCH:
	    stemmer_setup = setup_french_stemmer;
	    stemmer_stem = french_stem;
	    stemmer_closedown = closedown_french_stemmer;
	    break;
	case STEMLANG_GERMAN:
	    stemmer_setup = setup_german_stemmer;
	    stemmer_stem = german_stem;
	    stemmer_closedown = closedown_german_stemmer;
	    break;
	case STEMLANG_ITALIAN:
	    stemmer_setup = setup_italian_stemmer;
	    stemmer_stem = italian_stem;
	    stemmer_closedown = closedown_italian_stemmer;
	    break;
	case STEMLANG_PORTUGUESE:
	    stemmer_setup = setup_portuguese_stemmer;
	    stemmer_stem = portuguese_stem;
	    stemmer_closedown = closedown_portuguese_stemmer;
	    break;
	case STEMLANG_SPANISH:
	    stemmer_setup = setup_spanish_stemmer;
	    stemmer_stem = spanish_stem;
	    stemmer_closedown = closedown_spanish_stemmer;
	    break;
	case STEMLANG_PORTER:
	    stemmer_setup = setup_porter_stemmer;
	    stemmer_stem = porter_stem;
	    stemmer_closedown = closedown_porter_stemmer;
	    break;
	default:
	    break;
    }
    stemmer_data = stemmer_setup();
    // STEMLANG_NULL shouldn't be passed in here.
    Assert(stemmer_setup != 0);
}

stemmer_language
OmStem::Internal::get_stemtype(string language)
{
    return stringToTypeMap<stemmer_language>::get_type(language);  
}

string
OmStem::Internal::stem_word(string word) const
{
    int len = word.length();
    if(len == 0) return "";
    return string(stemmer_stem(stemmer_data, word.data(), 0, len - 1));
}

///////////////////////
// Methods of OmStem //
///////////////////////

OmStem::OmStem(string language)
	: internal(0)
{
    internal = new OmStem::Internal(language);
}

OmStem::~OmStem()
{
    delete internal;
}

OmStem::OmStem(const OmStem &other)
{
    // FIXME
    throw OmUnimplementedError("OmStem::OmStem(const OmStem &) unimplemented");
}

void
OmStem::operator=(const OmStem &other)
{
    // FIXME
    throw OmUnimplementedError("OmStem::operator=() unimplemented");
}

string
OmStem::stem_word(string word) const
{
    OmLockSentry locksentry(internal->mutex);
    return internal->stem_word(word);
}

vector<string>
OmStem::get_available_languages()
{
    // FIXME
    throw OmUnimplementedError("OmStem::get_available_languages() unimplemented");
}
