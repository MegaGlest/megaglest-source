-- MojoSetup; a portable, flexible installation application.
--
-- Please see the file LICENSE.txt in the source's root directory.
--
--  This file written by Ryan C. Gordon.
--
-- Lines starting with "--" are comments in this file.
--
-- You should add your installer's strings to this file; localization.lua is
--  for strings used internally by MojoSetup itself. Your app can override any
--  individual string in that file, though.
--
-- NOTE: If you care about Unicode or ASCII chars above 127, this file _MUST_
--  be UTF-8 encoded! If you think you're using a certain high-ASCII codepage,
--  you're wrong!
--
-- Most of the MojoSetup table isn't set up when this code is run, so you
--  shouldn't rely on any of it. For most purposes, you should treat this
--  file more like a data description language and less like a turing-complete
--  scripting language.
--
-- The format of an entry looks like this:
--
--  ["Hello"] = {
--    es = "Hola",
--    de = "Hallo",
--    fr = "Bonjour",
--  };
--
-- So you just fill in the translation of the English for your language code.
--  Note that locales work, too:
--
--  ["Color"] = {
--    en_GB = "Colour",
--  };
--
-- Specific locales are favored, falling back to specific languages, eventually
--  ending up on the untranslated version (which is technically en_US).
--
-- Whenever you see a %x sequence, that is replaced with a string at runtime.
--  So if you see, ["Hello, %0, my name is %1."], then this might become
--  "Hello, Alice, my name is Bob." at runtime. If your culture would find
--  introducing yourself second to be rude, you might translate this to:
--  "My name is %1, hello %0." If you need a literal '%' char, write "%%":
--  "Operation is %0%% complete" might give "Operation is 3% complete."
--  All strings, from your locale or otherwise, are checked for formatter
--  correctness at startup. This is to prevent the installer working fine
--  in all reasonable tests, then finding out that one guy in Ghana has a
--  crashing installer because his localization forgot to add a %1 somewhere.
--
-- Occasionally you might see a "\n" ... that's a newline character. "\t" is
--  a tab character, and "\\" turns into a single "\" character.
--
-- The table you create here goes away shortly after creation, as the relevant
--  parts of it get moved somewhere else. You should call MojoSetup.translate()
--  to get the proper translation for a given string.
--
-- Questions about the intent of a specific string can go to Ryan C. Gordon
--  (icculus@icculus.org).

MojoSetup.applocalization = {
};

-- end of app_localization.lua ...



