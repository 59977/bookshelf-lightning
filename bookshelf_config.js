'use strict';

const ColorPicker = window.Colorful.HexColorPicker;
const e = React.createElement;

function Led({ color, selected, setSelected }) {
  var classes = ['led'];  
  return e(
    'div', {       
      className: classes.join(' '),
      style: { 
        backgroundColor: color        
      }       
    },
    ''
  );  
}

function LedEditor({ color, selected, setSelected }) {
  var classes = ['led-editor'];
  if (selected) {
    classes.push('led-selected');
  }
  return e(
    'div', { 
      onClick: () => setSelected(!selected), 
      className: classes.join(' '),
      style: { 
        backgroundColor: color        
      }       
    },
    ''
  );  
}

function LedStripEditor({ leds, pasteAll }) {
  var ledElements = leds.map((l, i) => e(LedEditor, { key: i, color: l.colorState[0], selected: l.selectedState[0], setSelected: l.selectedState[1] }));
  const [pickerColor, setPickerColor] = React.useState("#ffc040");
  
  function setColor() {
    for (var led of leds.filter(l => l.selectedState[0])) {
      led.colorState[1](pickerColor);
    }
  }

  function turnOff() {
    for (var led of leds.filter(l => l.selectedState[0])) {
      led.colorState[1]("#000000");
    }
  }

  function clearSelection() {
    for (var led of leds.filter(l => l.selectedState[0])) {
      led.selectedState[1](false);
    }    
  }

  function selectAll() {
    for (var led of leds.filter(l => !l.selectedState[0])) {
      led.selectedState[1](true);
    } 
  }

  function pasteToAll() {
    pasteAll();
  }
  
  return e(
    'div', {
      className: 'ledstrip-editor'
    }, 
    e('div', { className: 'ledstrip-editor-tools' },  
      e(ColorPicker, { color: pickerColor, onChange: setPickerColor }),    
      e('div', { className: "editor-buttons-wrap" },
        e('button', { onClick: setColor, className: "button button-editor button-set" }, "Set"),  
        e('button', { onClick: turnOff, className: "button button-editor button-turn-off" }, "Turn off"),  
        e('button', { onClick: selectAll, className: "button button-editor button-select-all" }, "Select"),
        e('button', { onClick: clearSelection, className: "button button-editor button-select-clear" }, "Deselect"),
        e('button', { onClick: pasteToAll, className: "button button-editor button-select-clear" }, "Paste to all"))), 
    e('div', { className: 'ledstrip-editor-leds-wrapper' },  
      ledElements)     
  );    
}

function LedStrip({ onClick, leds }) {
  var ledElements = leds.map((l, i) => e(Led, { key: i, color: l.colorState[0], selected: l.selectedState[0], setSelected: l.selectedState[1] }));
  
  return e(
    'div', {
      className: 'ledstrip',
      onClick: onClick
    },              
    ledElements,
  );    
}

function LedStripArray({ strips }) {
  const [isFirst, setIsFirst] = React.useState(true);
  const [dimLevel, setDimLevel] = React.useState(1.0);
  const [selectedPatch, setSelectedPatch] = React.useState();
  const ledProps = (size) => Array(size).fill().map((_) => ({ colorState: React.useState("#000000"), selectedState: React.useState(false) }));
  const stripState = strips.map((s) => ledProps(s.size));
    
  React.useEffect(() => {
    if (isFirst) {
      setIsFirst(false);
      console.log("load");
      load();
    } else {
      save();
      console.log("save");
    }
  });    

  let patches = [];
  for (let i = 0; i < strips.length; i++) {
    let strip = strips[i];
    if (strip.patches) {
      for (let patch of strip.patches) {
        patches.push(e(LedStrip, { onClick: () => setSelectedPatch([i, patch]), key: patches.length, leds: stripState[i].slice(patch.start, patch.start + patch.size) }));        
      }
    } else {
      patches.push(e(LedStrip, { onClick: () => setSelectedPatch([i, null]), key: patches.length, leds: stripState[i] }));
    }
  }

  let editor = [];
  if (selectedPatch) {
    let i = selectedPatch[0];
    if (selectedPatch[1]) {
      let patch = selectedPatch[1];
      editor.push(e(LedStripEditor, { key: patches.length, leds: stripState[i].slice(patch.start, patch.start + patch.size), pasteAll: pasteSelectedToAllPatches }))
    } else {
      editor.push(e(LedStripEditor, { key: patches.length, leds: stripState[i], pasteAll: pasteSelectedToAllPatches }));
    }
  }

  function pasteSelectedToAllPatches() {
    if (selectedPatch) {
      let selectedIndex = selectedPatch[0];
      let patch = selectedPatch[1];      
      let patchState = patch
        ? stripState[selectedIndex].slice(patch.start, patch.start + patch.size)
        : stripState[selectedIndex];        

      for (let i = 0; i < strips.length; i++) {
        let strip = strips[i];
        if (strip.patches) {
          for (let patch of strip.patches) {
            for (let j = 0; j < patch.size; j++) {
              stripState[i][j + patch.start].colorState[1](patchState[j].colorState[0]);
            }
          }
        } else {
          for (let j = 0; j < stripState[i].length; j++) {
            stripState[i][j].colorState[1](patchState[j].colorState[0]);
          }
        }
      }
    }
  }

  async function light() {
    let i = 0;
    for (var strip of stripState) {
      const colors = strip.map(l => l.colorState[0].slice(1));  
      const newColors = colors.map(c => mapColor(c, dimLevel));
      const body = newColors.map(c => '00' + c).join('\r\n');  
      await putLedState(i, body).then(r => console.log(r));      
      i++;
    }
  }

  async function off() {
    let i = 0;
    for (var strip of stripState) {
      const body = strip.map(l => '00000000').join('\r\n');  
      await putLedState(i, body).then(r => console.log(r));      
      i++;
    }
  }

  function dim(delta) {
    var newDimLevel = dimLevel + delta;
    newDimLevel = newDimLevel > 1 ? 1 : newDimLevel;
    newDimLevel = newDimLevel < 0 ? 0 : newDimLevel;
    setDimLevel(newDimLevel);    
  }

  async function save() {
    localStorage.setItem("strips.state", JSON.stringify(stripState));
    console.log("Save");
  }

  async function load() {
    const newStripState = JSON.parse(localStorage.getItem("strips.state"));
    for (var i = 0; i < stripState.length; i++) {
      var strip = stripState[i];
      for (var j = 0; j < strip.length; j++) {
        var led = strip[j];
        led.colorState[1](newStripState[i][j].colorState[0]);
        led.selectedState[1](newStripState[i][j].selectedState[0]);
      }
    }
  }

  async function putLedState(index, body) {
    console.log(body);
    const response = await fetch('http://192.168.1.139/api/neopixels/' + index, {
      method: 'PUT', // *GET, POST, PUT, DELETE, etc.
      mode: 'cors', // no-cors, *cors, same-origin
      headers: {
        'Content-Type': 'text/plain'
      },
      redirect: 'error', // manual, *follow, error
      body: body 
    });
    return response;
  }

  function mapColor(c, dimLevel) {
    const r = parseInt(c.slice(0, 2), 16); 
    const g = parseInt(c.slice(2, 4), 16); 
    const b = parseInt(c.slice(4, 6), 16); 

    const nc = {
      r: parseInt(Math.floor(Math.min(r * dimLevel, 255))),
      g: parseInt(Math.floor(Math.min(g * dimLevel, 255))),
      b: parseInt(Math.floor(Math.min(b * dimLevel, 255)))
    };

    return nc.r.toString(16).padStart(2, '0') + nc.g.toString(16).padStart(2, '0') + nc.b.toString(16).padStart(2, '0');
  }
  
  return e(
    'div', {
    },
    e('div', { className: 'editor-wrap' }, editor),      
    e('div', { className: 'buttons-wrap' },      
      e('button', { onClick: light, className: "button button-light" }, "Light"),
      e('button', { onClick: off, className: "button button-off" }, "Off"),
      e('button', { onClick: () => dim(-0.1), className: "button button-dimmer" }, "-"),
      e('div', { className: 'dim-level' }, (parseFloat(dimLevel) * 100).toFixed(0)+"%"),      
      e('button', { onClick: () => dim(0.1), className: "button button-brighter" }, "+")),
    e('div', { className: 'shelf' }, patches)
  );
}


const domContainer = document.querySelector('#bookshelf_config_container');
ReactDOM.render(e(
  LedStripArray, 
  { strips: [
    { size: 150, patches: [{start: 0, size: 46}, {start: 48, size: 46}, {start: 96, size: 46}] }, 
    { size: 150, patches: [{start: 0, size: 46}, {start: 48, size: 46}, {start: 96, size: 46}] },
    { size: 46 }, 
    { size: 46 }, 
    { size: 46 }] }), 
  domContainer);