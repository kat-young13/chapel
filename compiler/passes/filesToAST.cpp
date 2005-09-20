#include "driver.h"
#include "files.h"
#include "filesToAST.h"
#include "parser.h"
#include "stringutil.h"
#include "symbol.h"
#include "symtab.h"
#include "countTokens.h"
#include "yy.h"
#include "../traversals/fixup.h"

ModuleSymbol* prelude = NULL;

void FilesToAST::run(Vec<ModuleSymbol*>* modules) {
  // parse prelude
  Symboltable::parsePrelude();
  char* chplroot = sysdirToChplRoot(system_dir);
  prelude = ParseFile(glomstrings(2, chplroot, "/modules/standard/prelude.chpl"),
                      MOD_INTERNAL);

  // parse user files
  Symboltable::doneParsingPreludes();

  yydebug = debugParserLevel;

  ParseFile(glomstrings(2, chplroot, "/modules/standard/_chpl_complex.chpl"),
            MOD_STANDARD);
  ParseFile(glomstrings(2, chplroot, "/modules/standard/_chpl_file.chpl"),
            MOD_STANDARD);
  ParseFile(glomstrings(2, chplroot, "/modules/standard/_chpl_htuple.chpl"),
            MOD_STANDARD);

  ParseFile(glomstrings(2, chplroot, "/modules/standard/_chpl_seq.chpl"),
            MOD_STANDARD);

  ParseFile(glomstrings(2, chplroot, "/modules/standard/_chpl_standard.chpl"),
            MOD_STANDARD);

  int filenum = 0;
  char* inputFilename = NULL;

  while (inputFilename = nthFilename(filenum++)) {
    ParseFile(inputFilename, MOD_USER);
  }
  finishCountingTokens();

  Symboltable::doneParsingUserFiles();

  Pass* fixup = new Fixup();
  fixup->run(Symboltable::getModules(MODULES_ALL));
}
