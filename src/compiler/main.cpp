// spite compiler — statically links libLLVM + liblld, runs them in-process.
//
//   build IR (LLVM C++ API) -> emit object (LLVM MC) -> link (lld::mingw::link)
//
// This is the driver that will grow into the full spite frontend. For now the
// "frontend" is a hand-built IR module; the runtime archive (libspiter.a)
// is linked into every output.

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <lld/Common/Driver.h>

// LLVM 23 gates each lld driver behind this macro; declares lld::mingw::link.
LLD_HAS_DRIVER(mingw)

#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

// Absolute path to this executable (the compiler itself).
static std::filesystem::path self_exe_path() {
#ifdef _WIN32
    std::wstring buf(32768, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0) return {};
    buf.resize(n);
    return std::filesystem::path(buf);
#else
    // TODO(linux): readlink("/proc/self/exe")
    return std::filesystem::path(".");
#endif
}

// The runtime archive, installed at <prefix>/lib/libspite_rt.a, located relative
// to the compiler at <prefix>/bin/spite:  <exe_dir>/../lib/libspite_rt.a
static std::string runtime_archive_path() {
    return (self_exe_path().parent_path() / ".." / "lib" / "libspite_rt.a").string();
}

// The mingw-w64 sysroot that ships with llvm-mingw (where libkernel32.a lives).
static const char* SYSROOT_LIB =
    "C:/DevTools/llvm-23.1.3-mingw-w64-ucrt/x86_64-w64-mingw32/lib";

int main() {
    // ---- 1. initialize native targets ----
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // ---- 2. build the module (mock frontend) ----
    //   fn main(args: byte[][]) -> i32 { if args.len > 1 { 1 } else { 2 } }
    llvm::LLVMContext ctx;
    auto mod = std::make_unique<llvm::Module>("spite_poc", ctx);
    llvm::IRBuilder<> b(ctx);

    auto* fn = llvm::Function::Create(
        llvm::FunctionType::get(b.getInt32Ty(), {b.getPtrTy(), b.getInt64Ty()}, false),
        llvm::Function::GlobalValue::ExternalLinkage, "spite_main", mod.get());
    auto* entry = llvm::BasicBlock::Create(ctx, "entry", fn);
    auto* one   = llvm::BasicBlock::Create(ctx, "one",   fn);
    auto* two   = llvm::BasicBlock::Create(ctx, "two",   fn);

    b.SetInsertPoint(entry);
    auto* has_arg = b.CreateICmpSGT(fn->getArg(1), b.getInt64(1)); // argc > 1
    b.CreateCondBr(has_arg, one, two);

    b.SetInsertPoint(one);
    b.CreateRet(b.getInt32(1));
    b.SetInsertPoint(two);
    b.CreateRet(b.getInt32(2));

    if (llvm::verifyModule(*mod, &llvm::errs())) {
        std::cerr << "IR verification failed\n";
        return 1;
    }

    // ---- 3. emit object (LLVM MC) ----
    llvm::Triple triple("x86_64-w64-windows-gnu");
    mod->setTargetTriple(triple);
    std::string err;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, err);
    if (!target) { std::cerr << "lookupTarget: " << err << "\n"; return 1; }

    llvm::TargetOptions opts;
    auto* tm = target->createTargetMachine(triple, "generic", "", opts,
                                           std::optional<llvm::Reloc::Model>());
    mod->setDataLayout(tm->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream obj_out("spite_main.o", ec, llvm::sys::fs::OF_None);
    if (ec) { std::cerr << "open object: " << ec.message() << "\n"; return 1; }

    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, obj_out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "addPassesToEmitFile failed\n";
        return 1;
    }
    pm.run(*mod);
    obj_out.flush();
    std::cout << "emitted spite_main.o\n";

    // ---- 4. link in-process (lld mingw driver) ----
    std::string rt = runtime_archive_path();
    std::vector<const char*> args = {
        "ld.lld",
        "spite_main.o", rt.c_str(),
        "-o", "spite.exe",
        "-e", "spite_entry",
        "--subsystem", "console",
        "-lkernel32",
        "-L", SYSROOT_LIB,
    };
    bool ok = lld::mingw::link(args, llvm::outs(), llvm::errs(),
                               /*exitEarly=*/false, /*disableOutput=*/false);
    if (!ok) {
        std::cerr << "link failed\n";
        return 1;
    }
    std::cout << "linked spite.exe\n";
    return 0;
}
