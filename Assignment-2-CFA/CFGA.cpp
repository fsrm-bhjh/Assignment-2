/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            using State = std::pair<unsigned, std::stack<unsigned>>;
            std::set<State> visited;
            std::vector<unsigned> path;

            std::function<void(const ICFGNode *)> dfs = [&](const ICFGNode *node)
            {
                const State state(node->getId(), callStack);
                if (!visited.insert(state).second)
                    return;
                path.push_back(node->getId());

                if (node->getId() == snk)
                {
                    if (callStack.empty())
                        recordPath(path);
                }
                else
                {
                    for (const auto *edge : node->getOutEdges())
                    {
                        if (const auto *call = dyn_cast<CallCFGEdge>(edge))
                        {
                            const unsigned callsite = call->getCallSite()->getId();
                            auto pending = callStack;
                            bool recursive = false;
                            while (!pending.empty())
                            {
                                if (pending.top() == callsite)
                                {
                                    recursive = true;
                                    break;
                                }
                                pending.pop();
                            }
                            if (recursive)
                                continue;

                            callStack.push(callsite);
                            dfs(edge->getDstNode());
                            callStack.pop();
                        }
                        else if (const auto *ret = dyn_cast<RetCFGEdge>(edge))
                        {
                            const unsigned callsite = ret->getCallSite()->getId();
                            if (callStack.empty() || callStack.top() != callsite)
                                continue;

                            callStack.pop();
                            dfs(edge->getDstNode());
                            callStack.push(callsite);
                        }
                        else if (isa<IntraCFGEdge>(edge))
                        {
                            dfs(edge->getDstNode());
                        }
                    }
                }

                path.pop_back();
                visited.erase(state);
            };

            dfs(icfg->getICFGNode(src));
        }
}
