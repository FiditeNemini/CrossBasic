/*
* crossbasic-project-compiler.js
* Converts a projectTree JSON to CrossBasic code for views, modules, and classes.
*/

/* ───────── helpers ───────── */
function quoteIfString(val, type = '') {
  return type && type.toLowerCase() === 'string'
    ? `"${String(val).replace(/"/g, '\\"')}"`
    : val;
}

const rgbToCrossBasic = rgb => {
  /*  rgb(…)  →  &cRRGGBB    |   already-hex #RRGGBB  →  &cRRGGBB  */
  if (/^#([0-9a-f]{6})$/i.test(rgb)) return '&c' + rgb.slice(1);

  const m = rgb.match(/rgb\(\s*(\d+),\s*(\d+),\s*(\d+)\s*\)/i);
  if (!m) return '&c000000';
  const [r, g, b] = m.slice(1).map(n => parseInt(n, 10));
  return `&c${[r, g, b].map(n => n.toString(16).padStart(2, '0')).join('')}`;
};

const pxToNumber    = px => parseInt(String(px).replace(/px$/, ''), 10);
const toBoolLiteral = v  => (v === true || v === 'true' ? 'True' : 'False');

const propMap = {
  left : 'Left',  top : 'Top',  width: 'Width',  height: 'Height',
  parent: 'Parent',
  /* “text” handled per-control below */
  textcolor : 'TextColor',
  bold : 'Bold', underline : 'Underline', italic : 'Italic',
  fontname : 'FontName', fontsize : 'FontSize',
  enabled : 'Enabled', visible : 'Visible'
};

/* ───────── view generator ───────── */
function generateViewCode(view) {
  const v       = view.designer;
  const varName = view.name;
  const lines   = [];

  /* header */
  lines.push(`'Application Created with CrossBasic IDE`, ``);
  lines.push(`Var ${varName} As New XWindow`);
  lines.push(`${varName}.Width = ${v.width + 7}`);
  lines.push(`${varName}.Height = ${v.height + 32}`);
  lines.push(`${varName}.BackgroundColor = ${rgbToCrossBasic(v.backgroundColor)}`);
  lines.push(`${varName}.ViewType = ${v.viewType}`);
  ['Close','HasMinimizeButton','HasMaximizeButton','HasFullScreenButton','HasTitleBar','Resizable'].forEach((opt, i) => {
    const key = [
      'hasCloseButton','hasMinimizeButton','hasMaximizeButton',
      'hasFullScreenButton','hasTitleBar','resizable'
    ][i];
    lines.push(`${varName}.${opt} = ${toBoolLiteral(v[key])}`);
  });
  lines.push(``);

  /* controls */
  (view.controls || []).forEach(ctrl => {
    const d     = ctrl.dynamic;
    const cname = d.name;

    lines.push(`Var ${cname} As New ${d.type}`);
    lines.push(`${cname}.Parent = ${d.parent || varName + '.Handle'}`);

    /* Left/Top/Width/Height – look on ctrl first, fallback to dynamic */
    ['left','top','width','height'].forEach(p => {
      const val = ctrl[p] ?? d[p];
      if (val != null) lines.push(`${cname}.${propMap[p]} = ${pxToNumber(val)}`);
    });

    /* the rest of the attributes */
    Object.keys(d).forEach(key => {
      if (['id','type','name','left','top','width','height','parent','_events'].includes(key)) return;

      let val = d[key];
      if (key === 'textcolor')                               val = rgbToCrossBasic(val);
      else if (/^(fontsize|width|height|left|top)$/.test(key)) val = pxToNumber(val);
      else if (val === 'true' || val === 'false')              val = toBoolLiteral(val);
      else if (['text','fontname','caption'].includes(key))    val = `"${val}"`;

      /* special-case: XButton.Text → Caption, otherwise Text */
      const destProp =
        key === 'text'
          ? (d.type === 'XButton' ? 'Caption' : 'Text')
          : (key === 'caption' ? 'Caption' : (propMap[key] || key));

      lines.push(`${cname}.${destProp} = ${val}`);
    });
    lines.push(``);

    /* control-level events (unscoped) */
    (d._events || []).forEach(ev => {
      lines.push(`AddHandler(${cname}.${ev.eventName}, AddressOf(${cname}_${ev.eventName}))`, ``);
      const head = `${ev.returnType ? 'Function' : 'Sub'} ${cname}_${ev.eventName}` +
                   `(${ev.parameters || ''})${ev.returnType ? ' As '+ev.returnType : ''}`;
      lines.push(head);
      ev.content.trim().split('\n').forEach(l => lines.push(`\t${l}`));
      lines.push(`End ${ev.returnType ? 'Function' : 'Sub'}`, ``);
    });
  });

  /* ----- view-level Event Handlers (Opening / Closing) ---------------- */
  const evtGroup = (view.children || []).find(g => g.groupName === 'Event Handlers');

  if (evtGroup) {
    evtGroup.children.forEach(ev => {
      /* AddHandler line */
      lines.push(
        `AddHandler(${varName}.${ev.name}, AddressOf(${varName}_${ev.name}))`,
        ``
      );

      /* method stub */
      const head =
        `${ev.returnType ? 'Function' : 'Sub'} ${varName}_${ev.name}` +
        `(${ev.parameters || ''})` +
        (ev.returnType ? ' As ' + ev.returnType : '');

      lines.push(head);

      /* body – use designer text, but fall back to a Quit() on Closing */
      const body =
        (ev.content || '').trim() ||
        (ev.name === 'Closing' ? 'Quit()' : '');

      body.split('\n').forEach(l => lines.push(`\t${l}`));
      lines.push(`End ${ev.returnType ? 'Function' : 'Sub'}`, ``);
    });
  }

  /* view-level groups (Constants / Properties / Methods) */
  (view.children || []).forEach(group => {
    if (group.groupName === 'Event Handlers') return;  // already processed

    switch (group.groupName) {

      case 'Constants':
        group.children.forEach(c => {
          const def = quoteIfString(c.defaultValue, c.typeProp);
          lines.push(`Const ${c.constantName} As ${c.typeProp} = ${def}`, ``);
        });
        break;

      case 'Properties':
        group.children.forEach(p => {
          if (!p.propertyType) return;  // skip placeholders
          const def = quoteIfString(p.defaultVal, p.propertyType);
          lines.push(`Var ${p.propertyName} As ${p.propertyType} = ${def}`, ``);
        });
        break;

      case 'Methods':
        group.children.forEach(m => {
          const head = `${m.returnType ? 'Function' : 'Sub'} ${m.methodName}` +
                       `(${m.parameters || ''})${m.returnType ? ' As '+m.returnType : ''}`;
          lines.push(head);
          m.content.trim().split('\n').forEach(l => lines.push(`\t${l}`));
          lines.push(`End ${m.returnType ? 'Function' : 'Sub'}`, ``);
        });
        break;
    }
  });

  /* no .Show() / no loop here – handled globally */
  return lines.join('\n');
}

/* ───────── module / class generators ───────── */
function generateModuleCode(mod) {
  const name  = mod.name;
  const lines = [`Module ${name}`, ``];

  mod.children.forEach(group => {
    switch (group.groupName) {

      case 'Constants':
        group.children.forEach(c => {
          const def = quoteIfString(c.defaultValue, c.typeProp);
          lines.push(`${c.scope} Const ${c.constantName} As ${c.typeProp} = ${def}`, ``);
        });
        break;

      case 'Properties':
        group.children.forEach(p => {
          const def = quoteIfString(p.defaultVal, p.propertyType);
          lines.push(`${p.scope} Var ${p.propertyName} As ${p.propertyType} = ${def}`, ``);
        });
        break;

      case 'Methods':
        group.children.forEach(m => {
          const head = `${m.scope} ${m.returnType ? 'Function' : 'Sub'} ${m.methodName}` +
                       `(${m.parameters || ''})${m.returnType ? ' As '+m.returnType : ''}`;
          lines.push(head);
          m.content.trim().split('\n').forEach(l => lines.push(`\t${l}`));
          lines.push(`End ${m.returnType ? 'Function' : 'Sub'}`, ``);
        });
        break;
    }
  });

  lines.push(`End Module`);
  return lines.join('\n');
}

function generateClassCode(klass) {
  const name  = klass.name;
  const lines = [`Class ${name}`, ``];

  klass.children.forEach(group => {
    switch (group.groupName) {

      case 'Constants':
        group.children.forEach(c => {
          const def = quoteIfString(c.defaultValue, c.typeProp);
          lines.push(`${c.scope} Const ${c.constantName} As ${c.typeProp} = ${def}`, ``);
        });
        break;

      case 'Properties':
        group.children.forEach(p => {
          const def = quoteIfString(p.defaultVal, p.propertyType);
          lines.push(`${p.scope} Var ${p.propertyName} As ${p.propertyType} = ${def}`, ``);
        });
        break;

      case 'Methods':
        group.children.forEach(m => {
          const head = `${m.scope} ${m.returnType ? 'Function' : 'Sub'} ${m.methodName}` +
                       `(${m.parameters || ''})${m.returnType ? ' As '+m.returnType : ''}`;
          lines.push(head);
          m.content.trim().split('\n').forEach(l => lines.push(`\t${l}`));
          lines.push(`End ${m.returnType ? 'Function' : 'Sub'}`, ``);
        });
        break;
    }
  });

  lines.push(`End Class`);
  return lines.join('\n');
}

/* ───────── driver ───────── */
function generateCrossBasicFromProjectTree(treeObj) {
  const root = treeObj.projectTree;
  if (!root || !Array.isArray(root.children)) return '';

  const views   = root.children.filter(n => n.type === 'view'  ).map(generateViewCode);
  const modules = root.children.filter(n => n.type === 'module').map(generateModuleCode);
  const classes = root.children.filter(n => n.type === 'class' ).map(generateClassCode);

  const parts = [
    ...views,
    ...modules,
    ...classes
  ];

  /* show only the DefaultView (if provided) */
  const defaultView = root.properties?.defaultView?.trim();
  if (defaultView) parts.push(`${defaultView}.Show()`, ``);

  /* global event-loop at very end */
  parts.push(
    `While True`,
    `\tDoEvents(1)`,
    `Wend`
  );

  return parts.join('\n\n');
}
