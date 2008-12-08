//
// complex2record
//
// This pass changes the primitive complex type to a record.  As an
// optimization, unimplemented, it can be turned off to take advantage
// of C99 support of complex types.
//

#include "astutil.h"
#include "build.h"
#include "expr.h"
#include "passes.h"
#include "stmt.h"
#include "stringutil.h"
#include "symbol.h"

static ClassType*
buildComplexRecord(const char* name, Type* real) {
  ClassType* ct = new ClassType(CLASS_RECORD);
  TypeSymbol* ts = new TypeSymbol(name, ct);
  ct->fields.insertAtTail(new DefExpr(new VarSymbol("re", real)));
  ct->fields.insertAtTail(new DefExpr(new VarSymbol("im", real)));
  rootModule->block->insertAtTail(new DefExpr(ts));
  return ct;
}

#define complex2rec(t)                                          \
  ((t == dtComplex[COMPLEX_SIZE_64]) ? complex64 :              \
   ((t == dtComplex[COMPLEX_SIZE_128]) ? complex128 : 0))

#define complex2real(t)                                                 \
  ((t == dtComplex[COMPLEX_SIZE_64]) ? dtReal[FLOAT_SIZE_32] :         \
   ((t == dtComplex[COMPLEX_SIZE_128]) ? dtReal[FLOAT_SIZE_64] : 0))

void
complex2record() {
  ClassType* complex64 = buildComplexRecord("_complex64", dtReal[FLOAT_SIZE_32]);
  ClassType* complex128 = buildComplexRecord("_complex128", dtReal[FLOAT_SIZE_64]);

  complex64->refType = dtComplex[COMPLEX_SIZE_64]->refType;
  complex128->refType = dtComplex[COMPLEX_SIZE_128]->refType;

  dtComplex[COMPLEX_SIZE_64]->refType = NULL;
  dtComplex[COMPLEX_SIZE_128]->refType = NULL;

  dtComplex[COMPLEX_SIZE_64]->symbol->defPoint->remove();
  dtComplex[COMPLEX_SIZE_128]->symbol->defPoint->remove();

  forv_Vec(SymExpr, se, gSymExprs) {
    if (VarSymbol* var = toVarSymbol(se->var)) {
      if (is_complex_type(var->type)) {
        if (var->immediate) {
          ClassType* ct = complex2rec(se->var->type);
          VarSymbol* tmp = newTemp(ct);
          se->getStmtExpr()->insertBefore(new DefExpr(tmp));
          se->getStmtExpr()->insertBefore(new CallExpr(PRIM_SET_MEMBER, tmp, ct->getField(1), complex2real(se->var->type)->defaultValue));
          se->getStmtExpr()->insertBefore(new CallExpr(PRIM_SET_MEMBER, tmp, ct->getField(2), complex2real(se->var->type)->defaultValue));
          se->replace(new SymExpr(tmp));
        }
      }
    } else if (TypeSymbol* ts = toTypeSymbol(se->var)) {
      if (is_complex_type(ts->type)) {
        se->var = complex2rec(ts->type)->symbol;
      }
    }
  }

  forv_Vec(DefExpr, def, gDefExprs) {
    if (!isTypeSymbol(def->sym))
      if (is_complex_type(def->sym->type))
        def->sym->type = complex2rec(def->sym->type);
    if (FnSymbol* fn = toFnSymbol(def->sym))
      if (is_complex_type(fn->retType))
        fn->retType = complex2rec(fn->retType);
  }

  forv_Vec(CallExpr, call, gCallExprs) {
    if (call->isPrimitive(PRIM_GET_REAL)) {
      call->primitive = primitives[PRIM_GET_MEMBER];
      ClassType* ct = toClassType(call->get(1)->typeInfo());
      if (isReferenceType(ct))
        ct = toClassType(ct->getValueType());
      call->insertAtTail(ct->getField(1));
    } else if (call->isPrimitive(PRIM_GET_IMAG)) {
      call->primitive = primitives[PRIM_GET_MEMBER];
      ClassType* ct = toClassType(call->get(1)->typeInfo());
      if (isReferenceType(ct))
        ct = toClassType(ct->getValueType());
      call->insertAtTail(ct->getField(2));
    }
  }

  //
  // change arrays of complexes into arrays of new complex records
  //
  forv_Vec(TypeSymbol, ts, gTypeSymbols) {
    if (ts->hasFlag(FLAG_DATA_CLASS)) {
      if (TypeSymbol* nt = toTypeSymbol(ts->type->substitutions.v[0].value)) {
        if (is_complex_type(nt->type)) {
          Type* complexType = complex2rec(nt->type);
          Symbol* complexTypeSymbol = (complexType) ? complexType->symbol : 0;
          ts->type->substitutions.v[0].value = complexTypeSymbol;
        }
      }
    }
  }

}
