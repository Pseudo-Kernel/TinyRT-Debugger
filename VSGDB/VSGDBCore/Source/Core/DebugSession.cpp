#include <VSGDBCore/DebugSession.h>
#include <VSGDBCore/NullComponents.h>

namespace VSGDBCore
{
    class DebugSession final : public IDebugSession
    {
    public:
        DebugSession()
            : Target(CreateNullDebugTarget()),
            ModuleManager(CreateModuleManager()),
            SymbolManager(CreateNullSymbolManager()),
            ExpressionEvaluator(CreateSimpleExpressionEvaluator()),
            Disassembler(CreateNullDisassembler()),
            Assembler(CreateNullAssembler())
        {
        }

        IDebugTarget& GetTarget() override
        {
            return *Target;
        }

        IModuleManager& GetModuleManager() override
        {
            return *ModuleManager;
        }

        ISymbolManager& GetSymbolManager() override
        {
            return *SymbolManager;
        }

        IExpressionEvaluator& GetExpressionEvaluator() override
        {
            return *ExpressionEvaluator;
        }

        IDisassembler& GetDisassembler() override
        {
            return *Disassembler;
        }

        IAssembler& GetAssembler() override
        {
            return *Assembler;
        }

        DebugError Connect(
            const DebugTargetConfig& Config) override
        {
            return Target->Connect(Config);
        }

        DebugError Disconnect() override
        {
            return Target->Disconnect();
        }

    private:
        std::unique_ptr<IDebugTarget> Target;
        std::unique_ptr<IModuleManager> ModuleManager;
        std::unique_ptr<ISymbolManager> SymbolManager;
        std::unique_ptr<IExpressionEvaluator> ExpressionEvaluator;
        std::unique_ptr<IDisassembler> Disassembler;
        std::unique_ptr<IAssembler> Assembler;
    };

    std::unique_ptr<IDebugSession>
        CreateDebugSession()
    {
        return std::make_unique<DebugSession>();
    }
}