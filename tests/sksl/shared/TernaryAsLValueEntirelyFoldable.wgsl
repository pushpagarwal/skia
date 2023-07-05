struct FSIn {
  @builtin(front_facing) sk_Clockwise: bool,
  @builtin(position) sk_FragCoord: vec4<f32>,
};
struct FSOut {
  @location(0) sk_FragColor: vec4<f32>,
};
fn main(_skParam0: vec2<f32>) -> vec4<f32> {
  let coords = _skParam0;
  {
    var r: f32;
    var g: f32;
    r = 0.0;
    g = 1.0;
    return vec4<f32>(r, g, 0.0, 1.0);
  }
}
@fragment fn fragmentMain(_stageIn: FSIn) -> FSOut {
  var _stageOut: FSOut;
  _stageOut.sk_FragColor = main(_stageIn.sk_FragCoord.xy);
  return _stageOut;
}