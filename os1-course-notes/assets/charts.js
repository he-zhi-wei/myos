// charts.js — os1 讲义中的 ECharts 图表
// 数据全部来自源码：test/*.c 的用例条数、src/syscall.c 的 17 项系统调用。
(function () {
  if (typeof echarts === 'undefined') return;

  var css = getComputedStyle(document.documentElement);
  function v(name, fallback) {
    var x = css.getPropertyValue(name).trim();
    return x || fallback;
  }

  var ink = v('--ink', '#1B1F27');
  var muted = v('--muted', '#5A6070');
  var accent = v('--accent', '#1E6E5A');
  var accent2 = v('--accent2', '#B5651D');
  var rule = v('--rule', '#DED8C9');
  var bg2 = v('--bg2', '#F2EEE4');

  var base = {
    renderer: 'svg',
    animation: false,
    textStyle: {
      fontFamily: "'Bricolage Grotesque','Noto Sans SC',sans-serif",
      color: ink
    }
  };

  var charts = [];

  function init(id, option) {
    var el = document.getElementById(id);
    if (!el) return;
    var c = echarts.init(el, null, { renderer: base.renderer });
    c.setOption(option);
    charts.push(c);
  }

  // V7：四个测试组的用例条数（合计 27）
  init('chart-test-cases', {
    animation: false,
    textStyle: base.textStyle,
    grid: { left: 56, right: 24, top: 24, bottom: 40 },
    tooltip: {
      trigger: 'axis',
      appendToBody: true,
      backgroundColor: '#FFFFFF',
      borderColor: rule,
      textStyle: { color: ink, fontSize: 12 }
    },
    xAxis: {
      type: 'category',
      data: ['ISR / IRQ', 'PMM', 'PIT', 'Shell 命令'],
      axisLine: { lineStyle: { color: rule } },
      axisTick: { show: false },
      axisLabel: { color: muted, fontSize: 12 }
    },
    yAxis: {
      type: 'value',
      name: '用例数',
      nameTextStyle: { color: muted, fontSize: 11 },
      splitLine: { lineStyle: { color: rule, type: 'dashed' } },
      axisLabel: { color: muted, fontSize: 11 }
    },
    series: [{
      type: 'bar',
      barWidth: '42%',
      data: [11, 8, 4, 4],
      itemStyle: {
        color: accent,
        borderColor: bg2,
        borderWidth: 0
      },
      label: {
        show: true,
        position: 'top',
        color: ink,
        fontSize: 12,
        fontWeight: 700
      }
    }]
  });

  // V9：17 项系统调用的功能分组
  init('chart-syscall-groups', {
    animation: false,
    textStyle: base.textStyle,
    tooltip: {
      trigger: 'item',
      appendToBody: true,
      backgroundColor: '#FFFFFF',
      borderColor: rule,
      textStyle: { color: ink, fontSize: 12 },
      formatter: '{b}<br/>共 {c} 项（{d}%）'
    },
    legend: {
      orient: 'vertical',
      right: 8,
      top: 'middle',
      itemWidth: 10,
      itemHeight: 10,
      textStyle: { color: muted, fontSize: 12 }
    },
    series: [{
      type: 'pie',
      radius: ['42%', '68%'],
      center: ['42%', '50%'],
      avoidLabelOverlap: true,
      itemStyle: { borderColor: '#FFFFFF', borderWidth: 2 },
      label: { color: ink, fontSize: 12, formatter: '{c}' },
      labelLine: { lineStyle: { color: rule } },
      data: [
        { name: '进程与退出', value: 5, itemStyle: { color: accent } },
        { name: '控制台 I/O', value: 5, itemStyle: { color: accent2 } },
        { name: '时间', value: 3, itemStyle: { color: '#5B7FA6' } },
        { name: '文件', value: 3, itemStyle: { color: '#8A6BA8' } },
        { name: '程序装载', value: 1, itemStyle: { color: '#C2A24B' } }
      ]
    }]
  });

  function resizeAll() {
    for (var i = 0; i < charts.length; i++) charts[i].resize();
  }

  window.addEventListener('resize', resizeAll);
  window.addEventListener('load', resizeAll);
})();
