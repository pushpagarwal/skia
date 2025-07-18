               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main" %sk_FragColor
               OpExecutionMode %main OriginUpperLeft

               ; Debug Information
               OpName %_kGuardedDivideEpsilon "$kGuardedDivideEpsilon"  ; id %5
               OpName %sk_FragColor "sk_FragColor"                      ; id %13
               OpName %_UniformBuffer "_UniformBuffer"                  ; id %17
               OpMemberName %_UniformBuffer 0 "src"
               OpMemberName %_UniformBuffer 1 "dst"
               OpName %blend_color_saturation_Qhh3 "blend_color_saturation_Qhh3"    ; id %2
               OpName %blend_hslc_h4h2h4h4 "blend_hslc_h4h2h4h4"                    ; id %3
               OpName %alpha "alpha"                                                ; id %49
               OpName %sda "sda"                                                    ; id %56
               OpName %dsa "dsa"                                                    ; id %62
               OpName %l "l"                                                        ; id %68
               OpName %r "r"                                                        ; id %77
               OpName %_2_mn "_2_mn"                                                ; id %91
               OpName %_3_mx "_3_mx"                                                ; id %97
               OpName %_4_lum "_4_lum"                                              ; id %116
               OpName %_5_result "_5_result"                                        ; id %123
               OpName %_6_minComp "_6_minComp"                                      ; id %130
               OpName %_7_maxComp "_7_maxComp"                                      ; id %136
               OpName %main "main"                                                  ; id %4

               ; Annotations
               OpDecorate %blend_color_saturation_Qhh3 RelaxedPrecision
               OpDecorate %blend_hslc_h4h2h4h4 RelaxedPrecision
               OpDecorate %_kGuardedDivideEpsilon RelaxedPrecision
               OpDecorate %sk_FragColor RelaxedPrecision
               OpDecorate %sk_FragColor Location 0
               OpDecorate %sk_FragColor Index 0
               OpMemberDecorate %_UniformBuffer 0 Offset 0
               OpMemberDecorate %_UniformBuffer 0 RelaxedPrecision
               OpMemberDecorate %_UniformBuffer 1 Offset 16
               OpMemberDecorate %_UniformBuffer 1 RelaxedPrecision
               OpDecorate %_UniformBuffer Block
               OpDecorate %16 Binding 0
               OpDecorate %16 DescriptorSet 0
               OpDecorate %22 RelaxedPrecision
               OpDecorate %24 RelaxedPrecision
               OpDecorate %25 RelaxedPrecision
               OpDecorate %26 RelaxedPrecision
               OpDecorate %27 RelaxedPrecision
               OpDecorate %28 RelaxedPrecision
               OpDecorate %29 RelaxedPrecision
               OpDecorate %30 RelaxedPrecision
               OpDecorate %31 RelaxedPrecision
               OpDecorate %32 RelaxedPrecision
               OpDecorate %33 RelaxedPrecision
               OpDecorate %34 RelaxedPrecision
               OpDecorate %35 RelaxedPrecision
               OpDecorate %36 RelaxedPrecision
               OpDecorate %37 RelaxedPrecision
               OpDecorate %38 RelaxedPrecision
               OpDecorate %39 RelaxedPrecision
               OpDecorate %40 RelaxedPrecision
               OpDecorate %45 RelaxedPrecision
               OpDecorate %46 RelaxedPrecision
               OpDecorate %47 RelaxedPrecision
               OpDecorate %alpha RelaxedPrecision
               OpDecorate %51 RelaxedPrecision
               OpDecorate %52 RelaxedPrecision
               OpDecorate %53 RelaxedPrecision
               OpDecorate %54 RelaxedPrecision
               OpDecorate %55 RelaxedPrecision
               OpDecorate %sda RelaxedPrecision
               OpDecorate %57 RelaxedPrecision
               OpDecorate %58 RelaxedPrecision
               OpDecorate %59 RelaxedPrecision
               OpDecorate %60 RelaxedPrecision
               OpDecorate %61 RelaxedPrecision
               OpDecorate %dsa RelaxedPrecision
               OpDecorate %63 RelaxedPrecision
               OpDecorate %64 RelaxedPrecision
               OpDecorate %65 RelaxedPrecision
               OpDecorate %66 RelaxedPrecision
               OpDecorate %67 RelaxedPrecision
               OpDecorate %l RelaxedPrecision
               OpDecorate %69 RelaxedPrecision
               OpDecorate %70 RelaxedPrecision
               OpDecorate %76 RelaxedPrecision
               OpDecorate %r RelaxedPrecision
               OpDecorate %78 RelaxedPrecision
               OpDecorate %79 RelaxedPrecision
               OpDecorate %85 RelaxedPrecision
               OpDecorate %86 RelaxedPrecision
               OpDecorate %87 RelaxedPrecision
               OpDecorate %_2_mn RelaxedPrecision
               OpDecorate %92 RelaxedPrecision
               OpDecorate %93 RelaxedPrecision
               OpDecorate %94 RelaxedPrecision
               OpDecorate %95 RelaxedPrecision
               OpDecorate %96 RelaxedPrecision
               OpDecorate %_3_mx RelaxedPrecision
               OpDecorate %98 RelaxedPrecision
               OpDecorate %99 RelaxedPrecision
               OpDecorate %105 RelaxedPrecision
               OpDecorate %106 RelaxedPrecision
               OpDecorate %109 RelaxedPrecision
               OpDecorate %110 RelaxedPrecision
               OpDecorate %112 RelaxedPrecision
               OpDecorate %113 RelaxedPrecision
               OpDecorate %115 RelaxedPrecision
               OpDecorate %_4_lum RelaxedPrecision
               OpDecorate %117 RelaxedPrecision
               OpDecorate %122 RelaxedPrecision
               OpDecorate %_5_result RelaxedPrecision
               OpDecorate %124 RelaxedPrecision
               OpDecorate %125 RelaxedPrecision
               OpDecorate %126 RelaxedPrecision
               OpDecorate %127 RelaxedPrecision
               OpDecorate %128 RelaxedPrecision
               OpDecorate %129 RelaxedPrecision
               OpDecorate %_6_minComp RelaxedPrecision
               OpDecorate %131 RelaxedPrecision
               OpDecorate %132 RelaxedPrecision
               OpDecorate %133 RelaxedPrecision
               OpDecorate %134 RelaxedPrecision
               OpDecorate %135 RelaxedPrecision
               OpDecorate %_7_maxComp RelaxedPrecision
               OpDecorate %137 RelaxedPrecision
               OpDecorate %138 RelaxedPrecision
               OpDecorate %146 RelaxedPrecision
               OpDecorate %147 RelaxedPrecision
               OpDecorate %148 RelaxedPrecision
               OpDecorate %150 RelaxedPrecision
               OpDecorate %151 RelaxedPrecision
               OpDecorate %152 RelaxedPrecision
               OpDecorate %153 RelaxedPrecision
               OpDecorate %154 RelaxedPrecision
               OpDecorate %155 RelaxedPrecision
               OpDecorate %163 RelaxedPrecision
               OpDecorate %164 RelaxedPrecision
               OpDecorate %165 RelaxedPrecision
               OpDecorate %166 RelaxedPrecision
               OpDecorate %167 RelaxedPrecision
               OpDecorate %168 RelaxedPrecision
               OpDecorate %169 RelaxedPrecision
               OpDecorate %170 RelaxedPrecision
               OpDecorate %171 RelaxedPrecision
               OpDecorate %172 RelaxedPrecision
               OpDecorate %173 RelaxedPrecision
               OpDecorate %174 RelaxedPrecision
               OpDecorate %175 RelaxedPrecision
               OpDecorate %176 RelaxedPrecision
               OpDecorate %177 RelaxedPrecision
               OpDecorate %178 RelaxedPrecision
               OpDecorate %179 RelaxedPrecision
               OpDecorate %180 RelaxedPrecision
               OpDecorate %181 RelaxedPrecision
               OpDecorate %182 RelaxedPrecision
               OpDecorate %183 RelaxedPrecision
               OpDecorate %184 RelaxedPrecision
               OpDecorate %185 RelaxedPrecision
               OpDecorate %186 RelaxedPrecision
               OpDecorate %187 RelaxedPrecision
               OpDecorate %188 RelaxedPrecision
               OpDecorate %189 RelaxedPrecision
               OpDecorate %190 RelaxedPrecision
               OpDecorate %191 RelaxedPrecision
               OpDecorate %192 RelaxedPrecision
               OpDecorate %193 RelaxedPrecision
               OpDecorate %203 RelaxedPrecision
               OpDecorate %207 RelaxedPrecision

               ; Types, variables and constants
      %float = OpTypeFloat 32
%_ptr_Private_float = OpTypePointer Private %float
%_kGuardedDivideEpsilon = OpVariable %_ptr_Private_float Private    ; RelaxedPrecision
       %bool = OpTypeBool
      %false = OpConstantFalse %bool
%float_9_99999994en09 = OpConstant %float 9.99999994e-09
    %float_0 = OpConstant %float 0
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%sk_FragColor = OpVariable %_ptr_Output_v4float Output  ; RelaxedPrecision, Location 0, Index 0
%_UniformBuffer = OpTypeStruct %v4float %v4float        ; Block
%_ptr_Uniform__UniformBuffer = OpTypePointer Uniform %_UniformBuffer
         %16 = OpVariable %_ptr_Uniform__UniformBuffer Uniform  ; Binding 0, DescriptorSet 0
    %v3float = OpTypeVector %float 3
%_ptr_Function_v3float = OpTypePointer Function %v3float
         %21 = OpTypeFunction %float %_ptr_Function_v3float
    %v2float = OpTypeVector %float 2
%_ptr_Function_v2float = OpTypePointer Function %v2float
%_ptr_Function_v4float = OpTypePointer Function %v4float
         %44 = OpTypeFunction %v4float %_ptr_Function_v2float %_ptr_Function_v4float %_ptr_Function_v4float
%_ptr_Function_float = OpTypePointer Function %float
    %float_1 = OpConstant %float 1
        %114 = OpConstantComposite %v3float %float_0 %float_0 %float_0
%float_0_300000012 = OpConstant %float 0.300000012
%float_0_589999974 = OpConstant %float 0.589999974
%float_0_109999999 = OpConstant %float 0.109999999
        %121 = OpConstantComposite %v3float %float_0_300000012 %float_0_589999974 %float_0_109999999
%float_6_10351562en05 = OpConstant %float 6.10351562e-05
       %void = OpTypeVoid
        %195 = OpTypeFunction %void
        %197 = OpConstantComposite %v2float %float_0 %float_1
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
      %int_1 = OpConstant %int 1


               ; Function blend_color_saturation_Qhh3
%blend_color_saturation_Qhh3 = OpFunction %float None %21   ; RelaxedPrecision
         %22 = OpFunctionParameter %_ptr_Function_v3float   ; RelaxedPrecision

         %23 = OpLabel
         %26 =   OpLoad %v3float %22                ; RelaxedPrecision
         %27 =   OpCompositeExtract %float %26 0    ; RelaxedPrecision
         %28 =   OpLoad %v3float %22                ; RelaxedPrecision
         %29 =   OpCompositeExtract %float %28 1    ; RelaxedPrecision
         %25 =   OpExtInst %float %1 FMax %27 %29   ; RelaxedPrecision
         %30 =   OpLoad %v3float %22                ; RelaxedPrecision
         %31 =   OpCompositeExtract %float %30 2    ; RelaxedPrecision
         %24 =   OpExtInst %float %1 FMax %25 %31   ; RelaxedPrecision
         %34 =   OpLoad %v3float %22                ; RelaxedPrecision
         %35 =   OpCompositeExtract %float %34 0    ; RelaxedPrecision
         %36 =   OpLoad %v3float %22                ; RelaxedPrecision
         %37 =   OpCompositeExtract %float %36 1    ; RelaxedPrecision
         %33 =   OpExtInst %float %1 FMin %35 %37   ; RelaxedPrecision
         %38 =   OpLoad %v3float %22                ; RelaxedPrecision
         %39 =   OpCompositeExtract %float %38 2    ; RelaxedPrecision
         %32 =   OpExtInst %float %1 FMin %33 %39   ; RelaxedPrecision
         %40 =   OpFSub %float %24 %32              ; RelaxedPrecision
                 OpReturnValue %40
               OpFunctionEnd


               ; Function blend_hslc_h4h2h4h4
%blend_hslc_h4h2h4h4 = OpFunction %v4float None %44     ; RelaxedPrecision
         %45 = OpFunctionParameter %_ptr_Function_v2float   ; RelaxedPrecision
         %46 = OpFunctionParameter %_ptr_Function_v4float   ; RelaxedPrecision
         %47 = OpFunctionParameter %_ptr_Function_v4float   ; RelaxedPrecision

         %48 = OpLabel
      %alpha =   OpVariable %_ptr_Function_float Function   ; RelaxedPrecision
        %sda =   OpVariable %_ptr_Function_v3float Function     ; RelaxedPrecision
        %dsa =   OpVariable %_ptr_Function_v3float Function     ; RelaxedPrecision
          %l =   OpVariable %_ptr_Function_v3float Function     ; RelaxedPrecision
         %72 =   OpVariable %_ptr_Function_v3float Function
          %r =   OpVariable %_ptr_Function_v3float Function     ; RelaxedPrecision
         %81 =   OpVariable %_ptr_Function_v3float Function
      %_2_mn =   OpVariable %_ptr_Function_float Function   ; RelaxedPrecision
      %_3_mx =   OpVariable %_ptr_Function_float Function   ; RelaxedPrecision
        %101 =   OpVariable %_ptr_Function_v3float Function
        %107 =   OpVariable %_ptr_Function_v3float Function
     %_4_lum =   OpVariable %_ptr_Function_float Function   ; RelaxedPrecision
  %_5_result =   OpVariable %_ptr_Function_v3float Function     ; RelaxedPrecision
 %_6_minComp =   OpVariable %_ptr_Function_float Function       ; RelaxedPrecision
 %_7_maxComp =   OpVariable %_ptr_Function_float Function       ; RelaxedPrecision
         %51 =   OpLoad %v4float %47                            ; RelaxedPrecision
         %52 =   OpCompositeExtract %float %51 3                ; RelaxedPrecision
         %53 =   OpLoad %v4float %46                            ; RelaxedPrecision
         %54 =   OpCompositeExtract %float %53 3                ; RelaxedPrecision
         %55 =   OpFMul %float %52 %54                          ; RelaxedPrecision
                 OpStore %alpha %55
         %57 =   OpLoad %v4float %46                ; RelaxedPrecision
         %58 =   OpVectorShuffle %v3float %57 %57 0 1 2     ; RelaxedPrecision
         %59 =   OpLoad %v4float %47                        ; RelaxedPrecision
         %60 =   OpCompositeExtract %float %59 3            ; RelaxedPrecision
         %61 =   OpVectorTimesScalar %v3float %58 %60       ; RelaxedPrecision
                 OpStore %sda %61
         %63 =   OpLoad %v4float %47                ; RelaxedPrecision
         %64 =   OpVectorShuffle %v3float %63 %63 0 1 2     ; RelaxedPrecision
         %65 =   OpLoad %v4float %46                        ; RelaxedPrecision
         %66 =   OpCompositeExtract %float %65 3            ; RelaxedPrecision
         %67 =   OpVectorTimesScalar %v3float %64 %66       ; RelaxedPrecision
                 OpStore %dsa %67
         %69 =   OpLoad %v2float %45                ; RelaxedPrecision
         %70 =   OpCompositeExtract %float %69 0    ; RelaxedPrecision
         %71 =   OpFUnordNotEqual %bool %70 %float_0
                 OpSelectionMerge %75 None
                 OpBranchConditional %71 %73 %74

         %73 =     OpLabel
                     OpStore %72 %67
                     OpBranch %75

         %74 =     OpLabel
                     OpStore %72 %61
                     OpBranch %75

         %75 = OpLabel
         %76 =   OpLoad %v3float %72                ; RelaxedPrecision
                 OpStore %l %76
         %78 =   OpLoad %v2float %45                ; RelaxedPrecision
         %79 =   OpCompositeExtract %float %78 0    ; RelaxedPrecision
         %80 =   OpFUnordNotEqual %bool %79 %float_0
                 OpSelectionMerge %84 None
                 OpBranchConditional %80 %82 %83

         %82 =     OpLabel
                     OpStore %81 %61
                     OpBranch %84

         %83 =     OpLabel
                     OpStore %81 %67
                     OpBranch %84

         %84 = OpLabel
         %85 =   OpLoad %v3float %81                ; RelaxedPrecision
                 OpStore %r %85
         %86 =   OpLoad %v2float %45                ; RelaxedPrecision
         %87 =   OpCompositeExtract %float %86 1    ; RelaxedPrecision
         %88 =   OpFUnordNotEqual %bool %87 %float_0
                 OpSelectionMerge %90 None
                 OpBranchConditional %88 %89 %90

         %89 =     OpLabel
         %94 =       OpCompositeExtract %float %76 0    ; RelaxedPrecision
         %95 =       OpCompositeExtract %float %76 1    ; RelaxedPrecision
         %93 =       OpExtInst %float %1 FMin %94 %95   ; RelaxedPrecision
         %96 =       OpCompositeExtract %float %76 2    ; RelaxedPrecision
         %92 =       OpExtInst %float %1 FMin %93 %96   ; RelaxedPrecision
                     OpStore %_2_mn %92
         %99 =       OpExtInst %float %1 FMax %94 %95   ; RelaxedPrecision
         %98 =       OpExtInst %float %1 FMax %99 %96   ; RelaxedPrecision
                     OpStore %_3_mx %98
        %100 =       OpFOrdGreaterThan %bool %98 %92
                     OpSelectionMerge %104 None
                     OpBranchConditional %100 %102 %103

        %102 =         OpLabel
        %105 =           OpCompositeConstruct %v3float %92 %92 %92  ; RelaxedPrecision
        %106 =           OpFSub %v3float %76 %105                   ; RelaxedPrecision
                         OpStore %107 %85
        %108 =           OpFunctionCall %float %blend_color_saturation_Qhh3 %107
        %109 =           OpVectorTimesScalar %v3float %106 %108     ; RelaxedPrecision
        %110 =           OpFSub %float %98 %92                      ; RelaxedPrecision
        %112 =           OpFDiv %float %float_1 %110                ; RelaxedPrecision
        %113 =           OpVectorTimesScalar %v3float %109 %112     ; RelaxedPrecision
                         OpStore %101 %113
                         OpBranch %104

        %103 =         OpLabel
                         OpStore %101 %114
                         OpBranch %104

        %104 =     OpLabel
        %115 =       OpLoad %v3float %101           ; RelaxedPrecision
                     OpStore %l %115
                     OpStore %r %67
                     OpBranch %90

         %90 = OpLabel
        %122 =   OpLoad %v3float %r                 ; RelaxedPrecision
        %117 =   OpDot %float %121 %122             ; RelaxedPrecision
                 OpStore %_4_lum %117
        %125 =   OpLoad %v3float %l                 ; RelaxedPrecision
        %124 =   OpDot %float %121 %125             ; RelaxedPrecision
        %126 =   OpFSub %float %117 %124            ; RelaxedPrecision
        %127 =   OpLoad %v3float %l                 ; RelaxedPrecision
        %128 =   OpCompositeConstruct %v3float %126 %126 %126   ; RelaxedPrecision
        %129 =   OpFAdd %v3float %128 %127                      ; RelaxedPrecision
                 OpStore %_5_result %129
        %133 =   OpCompositeExtract %float %129 0   ; RelaxedPrecision
        %134 =   OpCompositeExtract %float %129 1   ; RelaxedPrecision
        %132 =   OpExtInst %float %1 FMin %133 %134     ; RelaxedPrecision
        %135 =   OpCompositeExtract %float %129 2       ; RelaxedPrecision
        %131 =   OpExtInst %float %1 FMin %132 %135     ; RelaxedPrecision
                 OpStore %_6_minComp %131
        %138 =   OpExtInst %float %1 FMax %133 %134     ; RelaxedPrecision
        %137 =   OpExtInst %float %1 FMax %138 %135     ; RelaxedPrecision
                 OpStore %_7_maxComp %137
        %139 =   OpFOrdLessThan %bool %131 %float_0
                 OpSelectionMerge %141 None
                 OpBranchConditional %139 %140 %141

        %140 =     OpLabel
        %142 =       OpFUnordNotEqual %bool %117 %131
                     OpBranch %141

        %141 = OpLabel
        %143 =   OpPhi %bool %false %90 %142 %140
                 OpSelectionMerge %145 None
                 OpBranchConditional %143 %144 %145

        %144 =     OpLabel
        %146 =       OpCompositeConstruct %v3float %117 %117 %117   ; RelaxedPrecision
        %147 =       OpFSub %v3float %129 %146                      ; RelaxedPrecision
        %148 =       OpFSub %float %117 %131                        ; RelaxedPrecision
        %150 =       OpFAdd %float %148 %float_6_10351562en05       ; RelaxedPrecision
        %151 =       OpLoad %float %_kGuardedDivideEpsilon          ; RelaxedPrecision
        %152 =       OpFAdd %float %150 %151                        ; RelaxedPrecision
        %153 =       OpFDiv %float %117 %152                        ; RelaxedPrecision
        %154 =       OpVectorTimesScalar %v3float %147 %153         ; RelaxedPrecision
        %155 =       OpFAdd %v3float %146 %154                      ; RelaxedPrecision
                     OpStore %_5_result %155
                     OpBranch %145

        %145 = OpLabel
        %156 =   OpFOrdGreaterThan %bool %137 %55
                 OpSelectionMerge %158 None
                 OpBranchConditional %156 %157 %158

        %157 =     OpLabel
        %159 =       OpFUnordNotEqual %bool %137 %117
                     OpBranch %158

        %158 = OpLabel
        %160 =   OpPhi %bool %false %145 %159 %157
                 OpSelectionMerge %162 None
                 OpBranchConditional %160 %161 %162

        %161 =     OpLabel
        %163 =       OpLoad %v3float %_5_result     ; RelaxedPrecision
        %164 =       OpCompositeConstruct %v3float %117 %117 %117   ; RelaxedPrecision
        %165 =       OpFSub %v3float %163 %164                      ; RelaxedPrecision
        %166 =       OpFSub %float %55 %117                         ; RelaxedPrecision
        %167 =       OpVectorTimesScalar %v3float %165 %166         ; RelaxedPrecision
        %168 =       OpFSub %float %137 %117                        ; RelaxedPrecision
        %169 =       OpFAdd %float %168 %float_6_10351562en05       ; RelaxedPrecision
        %170 =       OpLoad %float %_kGuardedDivideEpsilon          ; RelaxedPrecision
        %171 =       OpFAdd %float %169 %170                        ; RelaxedPrecision
        %172 =       OpFDiv %float %float_1 %171                    ; RelaxedPrecision
        %173 =       OpVectorTimesScalar %v3float %167 %172         ; RelaxedPrecision
        %174 =       OpFAdd %v3float %164 %173                      ; RelaxedPrecision
                     OpStore %_5_result %174
                     OpBranch %162

        %162 = OpLabel
        %175 =   OpLoad %v3float %_5_result         ; RelaxedPrecision
        %176 =   OpLoad %v4float %47                ; RelaxedPrecision
        %177 =   OpVectorShuffle %v3float %176 %176 0 1 2   ; RelaxedPrecision
        %178 =   OpFAdd %v3float %175 %177                  ; RelaxedPrecision
        %179 =   OpFSub %v3float %178 %67                   ; RelaxedPrecision
        %180 =   OpLoad %v4float %46                        ; RelaxedPrecision
        %181 =   OpVectorShuffle %v3float %180 %180 0 1 2   ; RelaxedPrecision
        %182 =   OpFAdd %v3float %179 %181                  ; RelaxedPrecision
        %183 =   OpFSub %v3float %182 %61                   ; RelaxedPrecision
        %184 =   OpCompositeExtract %float %183 0           ; RelaxedPrecision
        %185 =   OpCompositeExtract %float %183 1           ; RelaxedPrecision
        %186 =   OpCompositeExtract %float %183 2           ; RelaxedPrecision
        %187 =   OpLoad %v4float %46                        ; RelaxedPrecision
        %188 =   OpCompositeExtract %float %187 3           ; RelaxedPrecision
        %189 =   OpLoad %v4float %47                        ; RelaxedPrecision
        %190 =   OpCompositeExtract %float %189 3           ; RelaxedPrecision
        %191 =   OpFAdd %float %188 %190                    ; RelaxedPrecision
        %192 =   OpFSub %float %191 %55                     ; RelaxedPrecision
        %193 =   OpCompositeConstruct %v4float %184 %185 %186 %192  ; RelaxedPrecision
                 OpReturnValue %193
               OpFunctionEnd


               ; Function main
       %main = OpFunction %void None %195

        %196 = OpLabel
        %198 =   OpVariable %_ptr_Function_v2float Function
        %204 =   OpVariable %_ptr_Function_v4float Function
        %208 =   OpVariable %_ptr_Function_v4float Function
         %10 =   OpSelect %float %false %float_9_99999994en09 %float_0
                 OpStore %_kGuardedDivideEpsilon %10
                 OpStore %198 %197
        %199 =   OpAccessChain %_ptr_Uniform_v4float %16 %int_0
        %203 =   OpLoad %v4float %199               ; RelaxedPrecision
                 OpStore %204 %203
        %205 =   OpAccessChain %_ptr_Uniform_v4float %16 %int_1
        %207 =   OpLoad %v4float %205               ; RelaxedPrecision
                 OpStore %208 %207
        %209 =   OpFunctionCall %v4float %blend_hslc_h4h2h4h4 %198 %204 %208
                 OpStore %sk_FragColor %209
                 OpReturn
               OpFunctionEnd
