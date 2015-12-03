#include <iostream>
#include <map>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <set>
#include <iterator>
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "CAT.h"

using namespace llvm;
using namespace std;

namespace {
  struct CatPass : public ModulePass {
    static char ID;
    Module *currentModule;

	// self defined data structures
	const vector<string> CAT_function_names = {"CAT_binary_add", "CAT_create_signed_value", "CAT_get_signed_value"};
	enum{CAT_binary_add_ID = 0, CAT_create_signed_value_ID = 1, CAT_get_signed_value_ID = 2};
	const vector<int> CAT_function_IDs = {CAT_binary_add_ID, CAT_create_signed_value_ID, CAT_get_signed_value_ID};
	map<Function *, string> CAT_functions;
	
    CatPass() : ModulePass(ID) {}

    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point
    bool doInitialization(Module &M) override{
        currentModule = &M;
		for (int i=0;i<CAT_function_names.size();i++){
			Function *CAT_Function;
			CAT_Function = M.getFunction(CAT_function_names[i].c_str());
			/*if (CAT_Function == NULL){
				errs()<<"CAT ERROR = CAT functions couldn't be found\n";
				abort();
			}*/
			CAT_functions[CAT_Function] = CAT_function_names[i];
		}
      return false;
    }

    // This function is invoked once per module compiled
    // The LLVM IR of the input functions is ready and it can be analyzed and/or transformed
    virtual bool runOnModule(Module &M) {
      errs() << "CATPass: Start\n" ;
      bool modified = false;
	  errs()<<"  START MODULE: "<<M.getName()<<"\n";
      
      
      
      map<Function *, set<Function *>> reverseCallGraph;
      map<Function *, Instruction *> callConstArg;
      map<Function *, int> callConstInt;
      
      //map<Function *, set<Function *>> callGraph;
      //map<Function *, Function *> singleCalls;
      //map<Function *, int> CallInstCount;
      
      // construct the call graph and reverse call graph
      for (auto& F : M){
          for (auto& B : F){
              for (auto& I : B){
                  if (auto call = dyn_cast<CallInst>(&I)){
                      Function *callee;
                      callee = call->getCalledFunction();
                      
                      //callGraph[&F].insert(callee);
                      reverseCallGraph[callee].insert(&F);
                  }
              }
          }
      }
      
      // record functions that are called only once
/*       for (auto it : reverseCallGraph){
          if (it.second.size() == 1){
              singleCalls[it.first] = *it.second.begin();
          }
      } */
	  
      // iterate over the whole module until all converges
      bool change = true;
      int iterationCount = 1;
      while (change){
          errs()<<"\n  beginning iteration number "<<iterationCount<<":\n";
          change = false;
          for (auto& F : M){
              if (intraProceduralConstantPropagation(F, callConstArg, callConstInt)){
                  change = true;
              }
          }
          iterationCount++;
      errs()<<"\n";
      }
      
      errs() << "CATPass: End\n\n" ;
    }
	  
    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override{
      //AU.setPreservesAll();
      AU.addRequiredTransitive<DependenceAnalysis>();
    }
	
private: 
	bool setCalleeID(Instruction *i, int *calleeID, CallInst **callInst){
		Function *callee;
		string calleeName;
		
		if (!isa<CallInst>(i)){
			return false;
		}
		(*callInst) = cast<CallInst>(i);
		
		callee = (*callInst)->getCalledFunction();
		if ((CAT_functions.find(callee) != CAT_functions.end())){
			calleeName = CAT_functions[callee];
		}
		if (calleeName.empty()){
			return false;
		}
		
		(*calleeID) = find(CAT_function_names.begin(), CAT_function_names.end(), calleeName) - CAT_function_names.begin();
		
		return true;
	}
    
    bool intraProceduralConstantPropagation(Function &F, map<Function *, Instruction*> &callConstArg, map<Function *, int> &callConstInt){
        bool modified = false;
        errs()<<"    START Function: "<<F.getName()<<"\n";
        
        // to avoid dependence analysis errors
        if(F.isDeclaration()){
            return modified;
        }
        DependenceAnalysis &deps = getAnalysis<DependenceAnalysis>(F);
        
        map<Value *, set<Instruction *>> defs;
        map<Instruction *, Value*> reverse_defs;
        map<Instruction *, set<Instruction *>> gen;
        map<Instruction *, set<Instruction *>> kill;
        map<Instruction *, set<Instruction *>> in;
        map<Instruction *, set<Instruction *>> out;
        map<Value *, Instruction *> valuesEscaped;
        
        // map a variable to instructions that define it
        bool firstBB = true;
        for (auto& B : F){
            bool firstI = true;
            for (auto& I : B){
                CallInst *callInst;
                int calleeID;
                
                if (firstBB && firstI){
                    if (callConstArg.count(&F)){
                        Instruction* defInst = callConstArg[&F];
                        
                        // assume functions other than CAT functions only have one argument, and it is a CATData
                        Function::arg_iterator arg = F.arg_begin();
                        if (Value* variable_modified = dyn_cast<Value>(arg)){
                            defs[variable_modified].insert(defInst);
                            reverse_defs[defInst] = variable_modified;
                        }
                    }
                }
                
                
                // record the variables that escape into the memory and the corresponding store instructions
                if (auto* storeInst = dyn_cast<StoreInst>(&I)){
                    valuesEscaped[storeInst->getValueOperand()] = &I;
                    continue;
                }
  			  
                if (isa<PHINode>(I)){
                    Value* variable_modified = &I;
                    Instruction* insn = &I;
                    defs[variable_modified].insert(insn);
                    reverse_defs[insn] = variable_modified;
                    continue;
                }
                
  			    if (!setCalleeID(&I, &calleeID, &callInst)){
  				    continue;
  			    }
                
                switch (calleeID){
                    case CAT_binary_add_ID:{
                        Value* variable_modified = callInst->getArgOperand(0);
  					    defs[variable_modified].insert(callInst);
                        reverse_defs[callInst] = variable_modified;
  				    }break;
  				  
  				    case CAT_create_signed_value_ID:{
  					    Value* variable_modified = dyn_cast<Value>(callInst);
  					    defs[variable_modified].insert(callInst);
                        reverse_defs[callInst] = variable_modified;
  				    }break;
  			    }
                firstI = false;
  		    }
            firstBB = false;
  	    }
        
        // for printing out the defs set
/*         for (auto& it : defs){
            errs()<<" Defs:\n";
            (it.first)->print(errs());
            errs()<<": ";
            for(auto& i :it.second){
                i->print(errs());
                errs()<<" ";
            }
            errs()<<"\n\n";
        } */
  	  
  	    // compute the GEN and KILL sets for each instruction
  	    for (auto& B : F){
  		    for (auto& I : B){
  			    CallInst *callInst;
  			    int calleeID;
                
                if (isa<PHINode>(I)){
                    Instruction* insn = &I;
                    gen[insn].insert(insn);
                    PHINode* P = cast<PHINode>(&I);
                    
                    Value* variable_modified = &I;
                    set<Instruction *> defset = defs[variable_modified];
  				    if (!defset.count(insn)){
                        errs()<<"defset not correct\n";
                        abort();
                    }
  
                    for (int i=0;i<P->getNumIncomingValues();i++){
                        Value* incomingValue = P->getIncomingValue(i);
                        set<Instruction *> union_set;
                        set_union(defset.begin(), defset.end(), defs[incomingValue].begin(), defs[incomingValue].end(), inserter(union_set, union_set.begin()));
                        defset = union_set;
                    }
                    defset.erase(insn);
  				    kill[insn] = defset;
                    continue;
                }
                
  			    if (!setCalleeID(&I, &calleeID, &callInst)){
  				    continue;
  			    }
  			  
  			    switch (calleeID){
  				    case CAT_binary_add_ID:{
  					    gen[callInst].insert(callInst);
  					  
  					    Value* variable_modified = callInst->getArgOperand(0);
  					    set<Instruction *> defset = defs[variable_modified];
  					    if (!defset.count(callInst)){
  						    errs()<<"defset not correct\n";
  						    abort();
  					    }
  					    defset.erase(callInst);
  					    kill[callInst] = defset;
  				    }break;
  				  
  				    case CAT_create_signed_value_ID:{
  					    gen[callInst].insert(callInst);
  					  
  					    Value* variable_modified = dyn_cast<Value>(callInst);
  					    set<Instruction *> defset = defs[variable_modified];
  					    if (!defset.count(callInst)){
  						    errs()<<"defset not correct\n";
  						    abort();
  					    }
  					    defset.erase(callInst);
  					    kill[callInst] = defset;
  				    }break;
  			    }
  		    }
  	    }
  
  	    // compute the IN and OUT sets of each instruction
  	    // first we set the IN and OUT set of all instructions to {}
  	    for (auto& B : F){
            for (auto& I : B){
                if(!out[&I].empty()){
                    out[&I].clear();
                }
                if(!in[&I].empty()){
                    in[&I].clear();
                }
            }
  	    }
  	  
        bool change = true;
        while (change){
            change = false;
            firstBB = true;
            for (auto& B : F){
                bool firstI = true;
                Instruction* previous;
                for (auto& I : B){
                    map<Instruction*, set<Instruction *>> outOld = out;
                    
                    // compute the IN set
                    
                    // for the first instruction of a function, if any constant propagated from caller, add it to the IN set
                    if (firstBB && firstI){
                        if (callConstArg.count(&F)){
                            in[&I].insert(callConstArg[&F]);
                        }
                    }
                    
                    // for the first instruction of a basic block, get all predecessor basic blocks
                    if (firstI){
                        for (pred_iterator PI = pred_begin(&B), E = pred_end(&B); PI != E; ++PI) {
                            BasicBlock *Pred = *PI;
                            Instruction* PredInstruction = Pred->getTerminator();
                            if (!out.count(PredInstruction)){
                                errs()<<"Error: No PredInstruction\n";
                                abort();
                            }
                            in[&I].insert(out[PredInstruction].begin(), out[PredInstruction].end());
                        }
                        
                        firstI = false;
                    }
                    else{
                        in[&I] = out[previous];
                    }
                    
                    // compute the OUT set
                    set<Instruction *> temp = in[&I];
                    if(kill.count(&I)){
                        for (auto it=kill[&I].begin(); it!=kill[&I].end(); it++){
                            temp.erase(*it);
                        }
                    }
                    if (gen.count(&I)){
                        for (auto it=gen[&I].begin(); it!=gen[&I].end(); it++){
                            temp.insert(*it);
                        }
                    }
                    out[&I] = temp;
                    
                    if(out!=outOld){
                        change = true;
                    }
                    previous = &I;
                }
                firstBB = false;
            }
        }
  	  
        // For printing the IN and OUT sets
/*         for (auto& B : F){
            for (auto& I : B){
                errs()<<"INSTRUCTION: ";
                I.print(errs());
                errs()<<"\n***************** IN\n{\n";
                    
                    for (auto it=in[&I].begin(); it!=in[&I].end(); it++){
                        errs()<<" ";
                        (*it)->print(errs());
                        errs()<<"\n";
                    }
                    errs()<<"}\n**************************************\n***************** OUT\n{\n";
                    for (auto it=out[&I].begin(); it!=out[&I].end(); it++){
                        errs()<<" ";
                        (*it)->print(errs());
                        errs()<<"\n";
                    }
                    errs()<<"}\n**************************************\n\n\n\n";
            }
        } */
        
        // Implement constant propagation for CAT functions
        map<Instruction *, Value *> replaceList;
        for (map<Value*, set<Instruction *>>::iterator it=defs.begin();it!=defs.end();it++){
  
            // iterate over every use of every Value in defs
            for (auto& U : (it->first)->uses()){
                User* user = U.getUser();
                if (auto *i = dyn_cast<Instruction>(user)){
                    CallInst *callInst = NULL;
                    int calleeID = 5;
                    
                    // we check if there is any memory dependencies for the variables that escape into the memory
                    if (valuesEscaped.count(it->first) && !F.isDeclaration()){
                         if (!checkDependencies(i, &deps, reverse_defs, in[i])){
                            continue;
                        } 
                    }
                  
                    setCalleeID(i, &calleeID, &callInst);
                    
                    // only consider CAT_get_signed_value functions or call functions
                    if (calleeID == CAT_create_signed_value_ID || calleeID == CAT_binary_add_ID || (callInst == NULL ^ isa<ReturnInst>(i))){
                        continue;
                    }
                    
                    int64_t c;
                    
                    // skip over ReturnInst for now
                    if (isa<ReturnInst>(i)){
                        continue;
                    }
                    
                    if (calleeID == CAT_get_signed_value_ID || isa<ReturnInst>(i)){
                        // check if all definitions of Value that reach this user are constants, if yes, the constant would be in c
                        
                        if(areconstdefs(in[i], (it->second), c)){
                            Value* constValue = ConstantInt::get(IntegerType::get(currentModule->getContext(), 64), c, false);
                        
                            errs() << "      CATPass:     found a constant propagation for instruction: ";
                            i->print(errs());
                            errs() << "\n";
                            errs() << "      CATPass:         replace with constant: "<<c<<"\n";
                        
                            // Put the instruction to modify and constant to use in the replaceList
                            replaceList.insert(pair<Instruction *, Value *>(i, constValue));
                        }
                    }
                    
                    // for functions that take in CAT variables as arguments
                    else{
                        if(areconstdefs(in[i], (it->second), c)){
                            Value* constValue = ConstantInt::get(IntegerType::get(currentModule->getContext(), 64), c, false);
                        
                            if (callConstArg.count(callInst->getCalledFunction())){
                                if (callConstInt[callInst->getCalledFunction()] != c){
                                    modified = false;
                                    continue;
                                }
                            }
                            else {
                                errs() << "      CATPass:     found a constant propagation for instruction: ";
                                i->print(errs());
                                errs() << "\n";
                                errs() << "      CATPass:         replace with constant: "<<c<<"\n";
                                errs() << "      NOTE: Might not propagate due to multiple definitions!\n";
                                
                                // record the function called and the value for propagation
                                Value* value = it->first;
                                if (Instruction* definst = dyn_cast<Instruction>(value)){
                                    callConstArg[callInst->getCalledFunction()] = definst;
                                    callConstInt[callInst->getCalledFunction()] = c;
                                }
                                else{
                                    auto it = defs[value].begin();
                                    definst = *it;
                                    callConstArg[callInst->getCalledFunction()] = definst;
                                    callConstInt[callInst->getCalledFunction()] = c;
                                }
                                modified = true;
                            }
                        }
                    }
                }
            }
        }   
        
        for (map<Instruction *, Value *>::iterator it=replaceList.begin();it!=replaceList.end();it++){
            Instruction* i= it->first;
            Value* constValue = it->second;
            BasicBlock::iterator ii(i);
            ReplaceInstWithValue(i->getParent()->getInstList(), ii, constValue);
            modified = true;
        }
        
        replaceList.clear();
  	    return modified;
    }
    
    void checkPhiNode(PHINode* P, set<int64_t> *PHItemp, bool &existNonConst){
        
        // for PHI nodes, check if all incoming values define the variable to be the same constant
        for (int i=0;i<P->getNumIncomingValues();i++){
            if (isa<PHINode>(P->getIncomingValue(i))){
                PHINode* PP = cast<PHINode>(P->getIncomingValue(i));
                
                // if the incoming value is another PHI node, we recursively check that node as well, and record any constant definitions
                checkPhiNode(PP, PHItemp, existNonConst);
                if (existNonConst == true){
                    return;
                }
            }
            
            else if (dyn_cast<CallInst>(P->getIncomingValue(i)) && !isa<PHINode>(P->getIncomingValue(i))){
                Value* incomingValue = cast<CallInst>(P->getIncomingValue(i))->getArgOperand(0);
                int64_t incomingConst = (cast<ConstantInt>(incomingValue))->getSExtValue();
                PHItemp->insert(incomingConst);
            }
                  
            // else the incoming value is not an instruction, it must be from outside the function
            else{
                existNonConst = true;
            }
        }
    }
    
    // check the inset of a use of a escaped variable, and pick out the instructions that alias with the current use we are looking at
    bool checkDependencies(Instruction* original, DependenceAnalysis *deps, map<Instruction*, Value*> &reverse_defs, const set<Instruction *> &inset){
        set<Value *> unique_def;
        
        // record the variables defined by instructions in the IN set, AND alias with the current use
        for (auto i : inset){
            if (deps->depends(i, original, false)){
                unique_def.insert(reverse_defs[i]);
            }
        }
        
        // return true if all aliases define the same variable
        if (unique_def.size() != 1){
            return false;
        }
        else{
            return true;
        }
    }
    
    bool areconstdefs(const set<Instruction *> &inset, const set<Instruction *> &defset, int64_t &c){
        bool areconst = true;
        int numDef = 0;
        set<Instruction *> intersection;

        set_intersection(inset.begin(), inset.end(), defset.begin(), defset.end(), inserter(intersection, intersection.begin()));
            
        if (intersection.empty()){
           return false;
        }
        
        vector<int64_t> temp;
        for (auto i : intersection){
            CallInst *callInst;
            int calleeID;
            
            if (isa<PHINode>(i)){
                bool existNonConst = false;
                set<int64_t> PHItemp;
                PHINode* P = cast<PHINode>(i);
                checkPhiNode(P, &PHItemp, existNonConst);
                
                if (PHItemp.size() == 1 && existNonConst == false){
                    numDef = 1;
                    temp.push_back(*PHItemp.begin());
                }
            }
            
            if (!setCalleeID(i, &calleeID, &callInst)){
                continue;
            }
            if(calleeID != CAT_create_signed_value_ID && calleeID != CAT_binary_add_ID){
                continue;
            }
            
            numDef++;
            Value* defvar = cast<CallInst>(i)->getArgOperand(0);
            
            if (!isa<ConstantInt>(defvar)){
                continue;
            }
            int64_t defnum = (cast<ConstantInt>(defvar))->getSExtValue();
            if (!(find(temp.begin(), temp.end(), defnum) != temp.end())){
                temp.push_back(defnum);
            }
        }     
        
        if (temp.size() == 0 ){
            areconst = false;
            
        }
        else if (temp.size() > 1){
            areconst = false;
        }
        else if (temp.size() < numDef){
            areconst = false;
        }
        else if (temp.size() == numDef){
            c = temp[0];
        }
        
        return areconst;
    }
    
/*      void getAnalysisUsage(AnalysisUsage &AU) const override{
      AU.addRequiredTransitive<DependenceAnalysis>();
    }  */
    
  };
}


// Next there is code to register your pass to the LLVM compiler
char CatPass::ID = 0;

static void registerCatPass (const PassManagerBuilder &, legacy::PassManagerBase &PM) {
  PM.add(new CatPass());
}

static RegisterStandardPasses RegisterMyPass (PassManagerBuilder::EP_EarlyAsPossible, registerCatPass);
static RegisterPass<CatPass> X("CAT", "A pass built for the CAT class at Northwestern");
