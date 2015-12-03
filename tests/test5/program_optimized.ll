; ModuleID = 'program_optimized.bc'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [17 x i8] c"H1: \09X    = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define i64 @foo2(i8* %d) #0 {
entry:
  %call = call i8* @CAT_create_signed_value(i64 10)
  ret i64 5
}

declare i8* @CAT_create_signed_value(i64) #1

declare i64 @CAT_get_signed_value(i8*) #1

; Function Attrs: nounwind uwtable
define i64 @foo(i8* %d) #0 {
entry:
  %call = call i8* @CAT_create_signed_value(i64 10)
  %call1 = call i64 @foo2(i8* %d)
  ret i64 %call1
}

; Function Attrs: nounwind uwtable
define i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %call = call i8* @CAT_create_signed_value(i64 5)
  %call1 = call i64 @foo2(i8* %call)
  %call2 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str, i32 0, i32 0), i64 %call1)
  ret i32 0
}

declare i32 @printf(i8*, ...) #1

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.7.0 (tags/RELEASE_370/final)"}
