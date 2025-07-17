import { invoke } from '@tauri-apps/api/core';

let clickCount = 0;

document.addEventListener('DOMContentLoaded', () => {
    const greetBtn = document.getElementById('greet-btn');
    const output = document.getElementById('output');

    greetBtn.addEventListener('click', async () => {
        clickCount++;
        
        try {
            // Update the output with a simple message
            output.textContent = `Button clicked ${clickCount} time${clickCount === 1 ? '' : 's'}! Tauri is working perfectly.`;
            
            // You can add Tauri API calls here, for example:
            // const result = await invoke('your_rust_function');
            
        } catch (error) {
            output.textContent = `Error: ${error}`;
        }
    });
});

// Log that the app is ready
console.log('LANA app is ready!'); 