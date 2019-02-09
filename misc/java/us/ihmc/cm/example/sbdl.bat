rem sbdl.bat (ScatterBotDemo Launcher)
@echo off
cls

set ROVMIND=us.ihmc.cm.example.ExampleRovingMind
set MOBPROV=us.ihmc.cm.aromaMobility.MobilityProviderImpl
set PLATPROV=us.ihmc.cm.example.ExamplePlatformProvider


launch anonymous guest us.ihmc.cm.aromaMobility.Launcher %ROVMIND% %MOBPROV% %PLATPROV% 
