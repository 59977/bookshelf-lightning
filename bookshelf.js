'use strict';

const Slider = window.Slider;
const e = React.createElement;

function App({ strips }) {
  const [status, setStatus] = React.useState("Not loaded");  
  const [state, setState] = React.useState({ x: 100, y: 100 });  

  async function load() {
    setStatus("Loading...");
    for (let i = 0; i < strips.length; i++) {
      const response = await getLedState(i).catch(r => console.log(r));
      strips[i].colors = (await response.text()).split('\r\n').filter(s => s.length >= 8).map(s => ({
        r: parseInt(s.slice(2, 4), 16), 
        g: parseInt(s.slice(4, 6), 16), 
        b: parseInt(s.slice(6, 8), 16) }));
      setStatus("Loading...(strip " + i + "loaded)");
    }
    
    setStatus("Loaded all strips");
  }

  function on() {
    setState({x: 100, y: 100});
    saveToDevice(100, 100);
  }

  function off() {
    setState({x: 0, y: 100});
    saveToDevice(0, 100);
  }

  function saveClick() {
    saveToDevice(state.x, state.y);
  }

  async function getLedState(index) {
    const response = await fetch('http://192.168.1.139/api/neopixels/' + index, {
      method: 'GET', // *GET, POST, PUT, DELETE, etc.
      mode: 'cors', // no-cors, *cors, same-origin      
      redirect: 'error', // manual, *follow, error     
    });
    return response;
  }

  async function saveToDevice(brightness, temperature) {
    console.log(brightness, temperature);
    for (let i = 0; i < strips.length; i++) {
      if (strips[i].colors) {  
        setStatus("Saving strip " + i + " to device...");
        const newColors = strips[i].colors.map(c => mapColor(c, brightness, temperature)).map(
          c => '00' + c.r.toString(16).padStart(2, '0') + c.g.toString(16).padStart(2, '0') + c.b.toString(16).padStart(2, '0'));
        await putLedState(i, newColors.join('\r\n')).catch(r => console.log(r));     
        //stripState[i][1](strips[i].colors.map(c => mapColor(c, brightness, temperature)).map(
        //  c => '#' + c.r.toString(16).padStart(2, '0') + c.g.toString(16).padStart(2, '0') + c.b.toString(16).padStart(2, '0')));
      }
    }

    setStatus("Saved");
  }  

  async function putLedState(index, body) {
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

  function dragEnd(sliderState) {
    //saveToDevice(sliderState.x, sliderState.y);
  }

  function mapColor(c, brightness, temperature) {
    return {
      r: Math.floor(Math.min(c.r * brightness * temperature / 10000, 255)),
      g: Math.floor(Math.min(c.g * brightness / 100, 255)),
      b: Math.floor(Math.min(c.b * brightness * (200 - temperature) / 10000, 255)),
    }
  }

  return e(
    'div', {
    },    
    e('button', { onClick: load }, "Load"),
    e('button', { onClick: on }, "On"),
    e('button', { onClick: off }, "Off"),
    e('button', { onClick: saveClick }, "Set"),
    e(Slider, {axis: "xy", xmax: 200, ymax: 200, x: state.x, y: state.y, onChange: setState, onDragEnd: dragEnd }),
    e('div', {}, 'Brightness: ' + state.x + 'Temperature: ' + state.y),
    e('div', {}, status),
  );
}


const domContainer = document.querySelector('#bookshelf_container');
ReactDOM.render(e(
  App, 
  { strips: [
    { size: 150 }, 
    { size: 150 },
    { size: 47 }, 
    { size: 47 }, 
    { size: 47 }] }), 
  domContainer);