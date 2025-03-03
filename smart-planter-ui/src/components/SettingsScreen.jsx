import React from 'react';
import { useNavigate } from 'react-router-dom';

function SettingsScreen() {
    const navigate = useNavigate();
    return (
        <div className="settings-screen">
            <button className="back-button" onClick={() => navigate(-1)}>Back</button>
            <h2>Settings and Notifications</h2>
            <div className="settings-item">
                <span>Water Levels:</span>
                <input type="range" min="0" max="100" defaultValue="70" />
            </div>
            <div className="settings-item">
                <span>Moisture Levels:</span>
                <input type="range" min="0" max="100" defaultValue="50" />
            </div>
            <div className="settings-item">
                <label><input type="checkbox" /> Email Reminders</label>
            </div>
            <div className="settings-item">
                <label><input type="checkbox" /> Water Tank Alert</label>
            </div>
            <div className="settings-item">
                <label><input type="checkbox" /> Plant Health Alerts</label>
            </div>
            <div className="settings-item">
                <label><input type="checkbox" /> Smart Watering</label>
            </div>
            <button className="button">Save</button>
        </div>
    );
}

export default SettingsScreen;
