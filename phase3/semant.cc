

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include "semant.h"
#include "utilities.h"
#include "symtab.h"


extern int semant_debug;
extern char *curr_filename;

SymbolTable<Symbol, tree_node> globalSymbolTable;

ClassTable *classtable;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}


ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cout) {

    /* Fill this in */
	allClasses = classes;
	install_basic_classes();
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);
	allClasses = append_Classes(allClasses, single_Classes(Object_class));

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);
	allClasses = append_Classes(allClasses, single_Classes(IO_class));

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);
	allClasses = append_Classes(allClasses, single_Classes(Int_class));

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);
	allClasses = append_Classes(allClasses, single_Classes(Bool_class));

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
	allClasses = append_Classes(allClasses, single_Classes(Str_class));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
}
 

void ClassTable::traverse() {
	globalSymbolTable.enterscope();
	for (int i = allClasses->first(); allClasses->more(i); i = allClasses->next(i)) {
		Class_ curClass = allClasses->nth(i);	// NOTE: this is a pointer. In "cool-tree.h": typedef class Class__class *Class_;
		if (curClass->getName() == Object || curClass->getName() == IO || curClass->getName() == Int || curClass->getName() == Bool || curClass->getName() == Str)
			continue;
		tree_node *v = globalSymbolTable.probe(curClass->getName());
		if (v == NULL)
			globalSymbolTable.addid(curClass->getName(), allClasses->nth(i));
		else
			classtable->semant_error() << curClass->getName() << " has already been defined. But you're redefining it here." << std::endl;
	}

	for (int i = allClasses->first(); allClasses->more(i); i = allClasses->next(i))
		allClasses->nth(i)->traverse();
	
	globalSymbolTable.exitscope();
}

void class__class::traverse() {
	globalSymbolTable.enterscope();
	for (int i = features->first(); features->more(i); i = features->next(i)) {
		tree_node *v = globalSymbolTable.probe(features->nth(i)->getName());
		if (v == NULL)
			globalSymbolTable.addid(features->nth(i)->getName(), features->nth(i));
		else
			classtable->semant_error() << features->nth(i)->getName() << " has already been defined. But you're redefining it here." << std::endl;
	}
	
	for (int i = features->first(); features->more(i); i = features->next(i))
		features->nth(i)->traverse();
	globalSymbolTable.exitscope();
}

void attr_class::traverse() {
	if (name == val || name == str_field)
		return;
	init->traverse();
	if (init->get_type() != No_type && init->get_type() != type_decl) {
		classtable->semant_error() << "attribute " << name << " has type " << type_decl->get_string() << " while, the expression has type " << init->get_type()->get_string() << std::endl;
	}
}

void method_class::traverse() {
	if (name == cool_abort ||
		name == idtable.add_string("copy") ||
		name == type_name ||
		name == out_int ||
		name == out_string ||
		name == in_int ||
		name == in_string ||
		name == length ||
		name == concat ||
		name == substr)
		return;
	globalSymbolTable.enterscope();
	for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
		tree_node *v = globalSymbolTable.probe(formals->nth(i)->getName());
		if (v == NULL)
			globalSymbolTable.addid(formals->nth(i)->getName(), formals->nth(i));
		else
			classtable->semant_error() << formals->nth(i)->getName() << " has already been defined. But you're redefining it here." << std::endl;
	}
	
	expr->traverse();
	
	if (expr->get_type() != return_type)
		classtable->semant_error() << name << " has return type " << return_type << " while the expression returned has type " << expr->get_type()->get_string() << endl;
	
	globalSymbolTable.exitscope();
}

void assign_class::traverse() {
	tree_node *v = globalSymbolTable.lookup(name);
	if (v == NULL) {
		classtable->semant_error() << name << " is undefined." << endl;		set_type(No_type);		return;
	}
	attr_class *fromAttr = dynamic_cast<attr_class*>(v);
	Formal fromFormal = dynamic_cast<Formal>(v);
	//attr_class *fromLet = dynamic_cast<let_ex>(v);
	//attr_class *fromCase = dynamic_cast<Formal_>(v);
	if (fromAttr == NULL && fromFormal == NULL) {
		classtable->semant_error() << name << " is NOT defined as object." << endl;		set_type(No_type);		return;
	}
	if (fromAttr != NULL && expr->get_type() != fromAttr->get_type())
		classtable->semant_error() << fromAttr->getName() << " has type " << fromAttr->get_type()->get_string() << " while the expression assigned has type " << expr->get_type()->get_string() << endl;		set_type(No_type);		return;

	if (fromFormal != NULL && expr->get_type() != fromFormal->get_type()) {
		classtable->semant_error() << fromFormal->getName() << " has type " << fromFormal->get_type()->get_string() << " while the expression assigned has type " << expr->get_type()->get_string() << endl;		set_type(No_type);		return;
		set_type(No_type);
	}
	else
		set_type(expr->get_type());
}

void plus_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() != Int || e2->get_type() != Int) {
		classtable->semant_error() << "You're doing arithmetic operations on non-integers." << endl;
		set_type(No_type);
	}
	else
		set_type(Int);
}

void sub_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() != Int || e2->get_type() != Int) {
		classtable->semant_error() << "You're doing arithmetic operations on non-integers." << endl;
		set_type(No_type);
	}
	else
		set_type(Int);
}

void mul_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() != Int || e2->get_type() != Int) {
		classtable->semant_error() << "You're doing arithmetic operations on non-integers." << endl;
		set_type(No_type);
	}
	else
		set_type(Int);
}

void divide_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() != Int || e2->get_type() != Int) {
		classtable->semant_error() << "You're doing arithmetic operations on non-integers." << endl;
		set_type(No_type);
	}
	else
		set_type(Int);
}

void comp_class::traverse() {
	e1->traverse();
	if (e1->get_type() != Int) {
		classtable->semant_error() << "The expression is not an integer and you're trying to get its complement" << endl;
		set_type(No_type);
	}
	else
		set_type(Int);
}

void lt_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() == e2->get_type() && e1->get_type() == Int)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Str)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Bool)
		set_type(Bool);
	if (e1->get_type() != e2->get_type()) {
		classtable->semant_error() << "The compared expressions don't have the same type." << endl;
		set_type(No_type);
	}
	else {
		classtable->semant_error() << "They're not of comparable types." << endl;
		set_type(No_type);
	}
}

void leq_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() == e2->get_type() && e1->get_type() == Int)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Str)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Bool)
		set_type(Bool);
	if (e1->get_type() != e2->get_type()) {
		classtable->semant_error() << "The compared expressions don't have the same type." << endl;
		set_type(No_type);
	}
	else {
		classtable->semant_error() << "They're not of comparable types." << endl;
		set_type(No_type);
	}
}

void eq_class::traverse() {
	e1->traverse();
	e2->traverse();
	if (e1->get_type() == e2->get_type() && e1->get_type() == Int)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Str)
		set_type(Bool);
	if (e1->get_type() == e2->get_type() && e1->get_type() == Bool)
		set_type(Bool);
	if (e1->get_type() != e2->get_type()) {
		classtable->semant_error() << "The compared expressions don't have the same type." << endl;
		set_type(No_type);
	}
	else {
		classtable->semant_error() << "They're not of comparable types." << endl;
		set_type(No_type);
	}
}

void neg_class::traverse() {
	e1->traverse();
	if (e1->get_type() != Bool) {
		classtable->semant_error() << "The expression is not a boolean expression." << endl;
		set_type(No_type);
	}
	else
		set_type(Bool);
}

void object_class::traverse() {
	tree_node *v = globalSymbolTable.lookup(name);
	if (v == NULL) {
		classtable->semant_error() << name << " is undefined." << endl;		set_type(No_type);		return;
	}
	attr_class *fromAttr = dynamic_cast<attr_class*>(v);
	Formal fromFormal = dynamic_cast<Formal>(v);
	//attr_class *fromLet = dynamic_cast<let_ex>(v);
	//attr_class *fromCase = dynamic_cast<Formal_>(v);
	if (fromAttr == NULL && fromFormal == NULL) {
		classtable->semant_error() << name << " is NOT defined as object." << endl;		set_type(No_type);		return;
	}
	if (fromAttr != NULL)
		set_type(fromAttr->get_type());
	else if (fromFormal != NULL)
		set_type(fromFormal->get_type());
}

void int_const_class::traverse() {
	set_type(Int);
}

void string_const_class::traverse() {
	set_type(Str);
}

void bool_const_class::traverse() {
	set_type(Bool);
}

void no_expr_class::traverse() {
	set_type(No_type);
}

void new__class::traverse() {
	tree_node *v = globalSymbolTable.lookup(type_name);
	if (v == NULL){
		classtable->semant_error() << type_name << " not defined." << std::endl;
		set_type(No_type);
	}
	else if (dynamic_cast<Class_>(v) == NULL) {
		classtable->semant_error() << type_name << " is not a type." << std::endl;
		set_type(No_type);
	}
	else
	{
		set_type(type_name);
	}
}

void cond_class::traverse() {
	pred->traverse();
	globalSymbolTable.enterscope();
	then_exp->traverse();
	globalSymbolTable.exitscope();
	
	globalSymbolTable.enterscope();
	else_exp->traverse();
	globalSymbolTable.enterscope();
	
	if(pred->get_type() != Bool) {
		classtable->semant_error() << "The precondition expression is not a boolean expression." << endl;
		set_type(No_type);
	}
	else if (then_exp->get_type() != else_exp->get_type()) {
		classtable->semant_error() << "The then and else expressions don't have the same type." << endl;
		set_type(No_type);
	}
	else
		set_type(then_exp->get_type());
}

void loop_class::traverse() {
	pred->traverse();
	globalSymbolTable.enterscope();
	body->traverse();
	globalSymbolTable.exitscope();
	if(pred->get_type() != Bool) {
		classtable->semant_error() << "The precondition expression is not a boolean expression." << endl;
		set_type(No_type);
	}
	else
		set_type(Object);
}

void isvoid_class::traverse() {
	if (e1->get_type() == No_type)
		set_type(No_type);
	else
		set_type(Bool);
}

void block_class::traverse() {
	globalSymbolTable.enterscope();
	Expression lastExpr = NULL;
	for (int i = body->first(); body->more(i); i = body->next(i)) {
		body->nth(i)->traverse();
		lastExpr = body->nth(i);
	}
	set_type(lastExpr->get_type());
	globalSymbolTable.exitscope();
}

void static_dispatch_class::traverse() {

	expr->traverse();
	for (int i = actual->first(); actual->more(i); i = actual->next(i))
		actual->nth(i)->traverse();
	Class_ exprType = dynamic_cast<Class_>(globalSymbolTable.lookup(type_name));
	if (exprType == NULL) {
		classtable->semant_error() << exprType << " is not a defined type." << endl;
		set_type(No_type);
		return;
	}
	if (expr->get_type() != type_name) {
		classtable->semant_error() << "Expression has type " << expr->get_type()->get_string() << " while it's static to be " << type_name << endl;
		set_type(No_type);
		return;
	}
	method_class *theMethod = NULL;
	Features features = exprType->getFeaturs();
	for (int i = features->first(); features->more(i); i = features->next(i)) {
		theMethod = dynamic_cast<method_class*>(features->nth(i));
		if (theMethod->getName() == name)
			break;
	}
	if (theMethod == NULL) {
		classtable->semant_error() << "No method " << name << " is defined in " << type_name << endl;
		set_type(No_type);
		return;
	}
	Formals methodFormals = theMethod->getFormals();
	if (methodFormals->len() != actual->len()) {
		classtable->semant_error() << "No method " << name << " with this number of parameters is defined in " << type_name << endl;
		set_type(No_type);
		return;
	}
	bool allGood = true;
	for (int i = methodFormals->first(), j = actual->first(); methodFormals->more(i); i = actual->next(i), j = actual->next(j))
		if (methodFormals->nth(i)->get_type() != actual->nth(i)->get_type()) {
			allGood = false;
			break;
		}
	if (allGood)
		set_type(type_name);
	else {
		classtable->semant_error() << "No method with such parameters' expressions is defined. Method name: " << name << endl;
		set_type(No_type);
	}
}

void dispatch_class::traverse() {
	
	expr->traverse();
	for (int i = actual->first(); actual->more(i); i = actual->next(i))
		actual->nth(i)->traverse();
	method_class * theMethod = dynamic_cast<method_class *>(globalSymbolTable.lookup(name));
	if (theMethod == NULL) {
		classtable->semant_error() << name->get_string() << " is not a defined." << endl;
		set_type(No_type);
		return;
	}
	Symbol type_name = theMethod->getReturnType();
	if (expr->get_type() != type_name) {
		classtable->semant_error() << "Expression has type " << expr->get_type()->get_string() << " while it's static to be " << type_name->get_string() << endl;
		set_type(No_type);
		return;
	}
	
	Formals methodFormals = theMethod->getFormals();
	if (methodFormals->len() != actual->len()) {
		classtable->semant_error() << "No method " << name << " with this number of parameters is defined in " << type_name->get_string() << endl;
		set_type(No_type);
		return;
	}
	bool allGood = true;
	for (int i = methodFormals->first(), j = actual->first(); methodFormals->more(i); i = actual->next(i), j = actual->next(j))
		if (methodFormals->nth(i)->get_type() != actual->nth(i)->get_type()) {
			allGood = false;
			break;
		}
	if (allGood)
		set_type(type_name);
	else {
		classtable->semant_error() << "No method with such parameters' expressions is defined. Method name: " << name->get_string() << endl;
		set_type(No_type);
	}
}
/*void branch_class::traverse() {
	globalSymbolTable.enterscope();
	tree_node *v = globalSymbolTable.probe(name);
	if (v == NULL){
		globalSymbolTable.addid(name, this);
	}else{
		classtable->semant_error() << name << " has been defined." << std::endl;
	}
	globalSymbolTable.exitscope();
}*/

/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */
    classtable->traverse();

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}


