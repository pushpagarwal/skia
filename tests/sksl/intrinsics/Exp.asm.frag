               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %_entrypoint_v "_entrypoint" %sk_FragColor
               OpExecutionMode %_entrypoint_v OriginUpperLeft

               ; Debug Information
               OpName %sk_FragColor "sk_FragColor"  ; id %3
               OpName %_UniformBuffer "_UniformBuffer"  ; id %8
               OpMemberName %_UniformBuffer 0 "inputVal"
               OpMemberName %_UniformBuffer 1 "expected"
               OpMemberName %_UniformBuffer 2 "colorGreen"
               OpMemberName %_UniformBuffer 3 "colorRed"
               OpName %_entrypoint_v "_entrypoint_v"    ; id %10
               OpName %main "main"                      ; id %2

               ; Annotations
               OpDecorate %main RelaxedPrecision
               OpDecorate %sk_FragColor RelaxedPrecision
               OpDecorate %sk_FragColor Location 0
               OpDecorate %sk_FragColor Index 0
               OpMemberDecorate %_UniformBuffer 0 Offset 0
               OpMemberDecorate %_UniformBuffer 0 RelaxedPrecision
               OpMemberDecorate %_UniformBuffer 1 Offset 16
               OpMemberDecorate %_UniformBuffer 1 RelaxedPrecision
               OpMemberDecorate %_UniformBuffer 2 Offset 32
               OpMemberDecorate %_UniformBuffer 2 RelaxedPrecision
               OpMemberDecorate %_UniformBuffer 3 Offset 48
               OpMemberDecorate %_UniformBuffer 3 RelaxedPrecision
               OpDecorate %_UniformBuffer Block
               OpDecorate %7 Binding 0
               OpDecorate %7 DescriptorSet 0
               OpDecorate %25 RelaxedPrecision
               OpDecorate %30 RelaxedPrecision
               OpDecorate %31 RelaxedPrecision
               OpDecorate %34 RelaxedPrecision
               OpDecorate %35 RelaxedPrecision
               OpDecorate %39 RelaxedPrecision
               OpDecorate %41 RelaxedPrecision
               OpDecorate %42 RelaxedPrecision
               OpDecorate %44 RelaxedPrecision
               OpDecorate %45 RelaxedPrecision
               OpDecorate %52 RelaxedPrecision
               OpDecorate %54 RelaxedPrecision
               OpDecorate %55 RelaxedPrecision
               OpDecorate %58 RelaxedPrecision
               OpDecorate %59 RelaxedPrecision
               OpDecorate %66 RelaxedPrecision
               OpDecorate %68 RelaxedPrecision
               OpDecorate %70 RelaxedPrecision
               OpDecorate %79 RelaxedPrecision
               OpDecorate %80 RelaxedPrecision
               OpDecorate %87 RelaxedPrecision
               OpDecorate %88 RelaxedPrecision
               OpDecorate %96 RelaxedPrecision
               OpDecorate %97 RelaxedPrecision
               OpDecorate %105 RelaxedPrecision
               OpDecorate %116 RelaxedPrecision
               OpDecorate %119 RelaxedPrecision
               OpDecorate %120 RelaxedPrecision

               ; Types, variables and constants
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%sk_FragColor = OpVariable %_ptr_Output_v4float Output  ; RelaxedPrecision, Location 0, Index 0
%_UniformBuffer = OpTypeStruct %v4float %v4float %v4float %v4float  ; Block
%_ptr_Uniform__UniformBuffer = OpTypePointer Uniform %_UniformBuffer
          %7 = OpVariable %_ptr_Uniform__UniformBuffer Uniform  ; Binding 0, DescriptorSet 0
       %void = OpTypeVoid
         %12 = OpTypeFunction %void
    %float_0 = OpConstant %float 0
    %v2float = OpTypeVector %float 2
         %16 = OpConstantComposite %v2float %float_0 %float_0
%_ptr_Function_v2float = OpTypePointer Function %v2float
         %20 = OpTypeFunction %v4float %_ptr_Function_v2float
       %bool = OpTypeBool
      %false = OpConstantFalse %bool
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
      %int_1 = OpConstant %int 1
     %v2bool = OpTypeVector %bool 2
    %v3float = OpTypeVector %float 3
     %v3bool = OpTypeVector %bool 3
     %v4bool = OpTypeVector %bool 4
    %float_1 = OpConstant %float 1
         %85 = OpConstantComposite %v2float %float_1 %float_1
         %94 = OpConstantComposite %v3float %float_1 %float_1 %float_1
        %103 = OpConstantComposite %v4float %float_1 %float_1 %float_1 %float_1
%_ptr_Function_v4float = OpTypePointer Function %v4float
      %int_2 = OpConstant %int 2
      %int_3 = OpConstant %int 3


               ; Function _entrypoint_v
%_entrypoint_v = OpFunction %void None %12

         %13 = OpLabel
         %17 =   OpVariable %_ptr_Function_v2float Function
                 OpStore %17 %16
         %19 =   OpFunctionCall %v4float %main %17
                 OpStore %sk_FragColor %19
                 OpReturn
               OpFunctionEnd


               ; Function main
       %main = OpFunction %v4float None %20         ; RelaxedPrecision
         %21 = OpFunctionParameter %_ptr_Function_v2float

         %22 = OpLabel
        %109 =   OpVariable %_ptr_Function_v4float Function
         %26 =   OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %30 =   OpLoad %v4float %26                ; RelaxedPrecision
         %31 =   OpCompositeExtract %float %30 0    ; RelaxedPrecision
         %25 =   OpExtInst %float %1 Exp %31        ; RelaxedPrecision
         %32 =   OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %34 =   OpLoad %v4float %32                ; RelaxedPrecision
         %35 =   OpCompositeExtract %float %34 0    ; RelaxedPrecision
         %36 =   OpFOrdEqual %bool %25 %35
                 OpSelectionMerge %38 None
                 OpBranchConditional %36 %37 %38

         %37 =     OpLabel
         %40 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %41 =       OpLoad %v4float %40            ; RelaxedPrecision
         %42 =       OpVectorShuffle %v2float %41 %41 0 1   ; RelaxedPrecision
         %39 =       OpExtInst %v2float %1 Exp %42          ; RelaxedPrecision
         %43 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %44 =       OpLoad %v4float %43            ; RelaxedPrecision
         %45 =       OpVectorShuffle %v2float %44 %44 0 1   ; RelaxedPrecision
         %46 =       OpFOrdEqual %v2bool %39 %45
         %48 =       OpAll %bool %46
                     OpBranch %38

         %38 = OpLabel
         %49 =   OpPhi %bool %false %22 %48 %37
                 OpSelectionMerge %51 None
                 OpBranchConditional %49 %50 %51

         %50 =     OpLabel
         %53 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %54 =       OpLoad %v4float %53            ; RelaxedPrecision
         %55 =       OpVectorShuffle %v3float %54 %54 0 1 2     ; RelaxedPrecision
         %52 =       OpExtInst %v3float %1 Exp %55              ; RelaxedPrecision
         %57 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %58 =       OpLoad %v4float %57            ; RelaxedPrecision
         %59 =       OpVectorShuffle %v3float %58 %58 0 1 2     ; RelaxedPrecision
         %60 =       OpFOrdEqual %v3bool %52 %59
         %62 =       OpAll %bool %60
                     OpBranch %51

         %51 = OpLabel
         %63 =   OpPhi %bool %false %38 %62 %50
                 OpSelectionMerge %65 None
                 OpBranchConditional %63 %64 %65

         %64 =     OpLabel
         %67 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %68 =       OpLoad %v4float %67            ; RelaxedPrecision
         %66 =       OpExtInst %v4float %1 Exp %68  ; RelaxedPrecision
         %69 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %70 =       OpLoad %v4float %69            ; RelaxedPrecision
         %71 =       OpFOrdEqual %v4bool %66 %70
         %73 =       OpAll %bool %71
                     OpBranch %65

         %65 = OpLabel
         %74 =   OpPhi %bool %false %51 %73 %64
                 OpSelectionMerge %76 None
                 OpBranchConditional %74 %75 %76

         %75 =     OpLabel
         %78 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %79 =       OpLoad %v4float %78            ; RelaxedPrecision
         %80 =       OpCompositeExtract %float %79 0    ; RelaxedPrecision
         %81 =       OpFOrdEqual %bool %float_1 %80
                     OpBranch %76

         %76 = OpLabel
         %82 =   OpPhi %bool %false %65 %81 %75
                 OpSelectionMerge %84 None
                 OpBranchConditional %82 %83 %84

         %83 =     OpLabel
         %86 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %87 =       OpLoad %v4float %86            ; RelaxedPrecision
         %88 =       OpVectorShuffle %v2float %87 %87 0 1   ; RelaxedPrecision
         %89 =       OpFOrdEqual %v2bool %85 %88
         %90 =       OpAll %bool %89
                     OpBranch %84

         %84 = OpLabel
         %91 =   OpPhi %bool %false %76 %90 %83
                 OpSelectionMerge %93 None
                 OpBranchConditional %91 %92 %93

         %92 =     OpLabel
         %95 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
         %96 =       OpLoad %v4float %95            ; RelaxedPrecision
         %97 =       OpVectorShuffle %v3float %96 %96 0 1 2     ; RelaxedPrecision
         %98 =       OpFOrdEqual %v3bool %94 %97
         %99 =       OpAll %bool %98
                     OpBranch %93

         %93 = OpLabel
        %100 =   OpPhi %bool %false %84 %99 %92
                 OpSelectionMerge %102 None
                 OpBranchConditional %100 %101 %102

        %101 =     OpLabel
        %104 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_1
        %105 =       OpLoad %v4float %104           ; RelaxedPrecision
        %106 =       OpFOrdEqual %v4bool %103 %105
        %107 =       OpAll %bool %106
                     OpBranch %102

        %102 = OpLabel
        %108 =   OpPhi %bool %false %93 %107 %101
                 OpSelectionMerge %113 None
                 OpBranchConditional %108 %111 %112

        %111 =     OpLabel
        %114 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_2
        %116 =       OpLoad %v4float %114           ; RelaxedPrecision
                     OpStore %109 %116
                     OpBranch %113

        %112 =     OpLabel
        %117 =       OpAccessChain %_ptr_Uniform_v4float %7 %int_3
        %119 =       OpLoad %v4float %117           ; RelaxedPrecision
                     OpStore %109 %119
                     OpBranch %113

        %113 = OpLabel
        %120 =   OpLoad %v4float %109               ; RelaxedPrecision
                 OpReturnValue %120
               OpFunctionEnd
