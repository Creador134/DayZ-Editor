


class COMImportData
{
	string name;
	ref array<ref Param3<string, vector, vector>> m_SceneObjects;
	
	void COMImportData()
	{
		m_SceneObjects = new array<ref Param3<string, vector, vector>>();
	}
}

class EditorWorldObject
{
	string m_Typename;
	vector m_Transform[4];
}

class EditorWorldData
{
	string MapName;
	vector CameraPosition[4];
	ref array<ref EditorWorldObject> WorldObjects;
	
	void EditorWorldData()
	{
		WorldObjects = new array<ref EditorWorldObject>();
	}	
}

enum ExportMode 
{
	TERRAINBUILDER,
	COMFILE, 
	EXPANSION,
	VPP
}

enum ImportMode
{
	MAPFILE, 
	COMFILE
}

enum HeightType 
{
	ABSOLUTE,
	RELATIVE
}

class ExportSettings
{
	static HeightType ExportHeightType;
}


enum FileDialogResult
{
	SUCCESS = 0,
	NOT_FOUND = 1,
	IN_USE = 2,
	UNKNOWN_ERROR = 100
}


class EditorFileManager
{


	static FileDialogResult Save(ref EditorWorldData data, string filename = "editor_save", string filedir = "$profile:Editor/")
	{
		string file = filedir + filename + ".dze";
		if (FileExist(file)) {
			
			GetEditor().GetUIManager().GetEditorUI().CreateDialog();
	
			
		}
		
		JsonFileLoader<ref EditorWorldData>.JsonSaveFile(file, data);
		return FileDialogResult.SUCCESS;
	}
	
	static FileDialogResult Open(out EditorWorldData data, string filename = "editor_save", string filedir = "$profile:Editor/")
	{
		string file = filedir + filename + ".dze";
		if (!FileExist(file)) {
			return FileDialogResult.NOT_FOUND;
		}
		
		JsonFileLoader<EditorWorldData>.JsonLoadFile(file, data);
		return FileDialogResult.SUCCESS;
	}
	
	static EditorWorldData Import(string filename, ImportMode mode = ImportMode.COMFILE)
	{
		EditorWorldData data = new EditorWorldData();

		switch (mode) {
			
			case (ImportMode.COMFILE): {
				
				COMImportData com_data = new COMImportData();
				JsonFileLoader<COMImportData>.JsonLoadFile(filename, com_data);
				
				foreach (ref Param3<string, vector, vector> param: com_data.m_SceneObjects) {
					Print("ImportFromFile::COMFILE::Import " + param.param1);
					vector transform[4];
					param.param3.RotationMatrixFromAngles(transform);
					transform[3] = param.param2;
					
					EditorWorldObject world_object = new EditorWorldObject();
					world_object.m_Typename = param.param1;
					world_object.m_Transform = transform;
					
					data.WorldObjects.Insert(world_object);
				}
			}
		}

		
		return data;
		
	}
	
	static void Export(EditorObjectSet export_objects, ExportMode mode = ExportMode.TERRAINBUILDER, string filename = "export", HeightType height_type = HeightType.RELATIVE)
	{
		Print("Exporting to File...");
		
		switch (mode) {
			
			case ExportMode.EXPANSION: {
				filename += ".map";
				break;
			}
			
			default: {
				filename += ".txt";
				break;
			}
			
		}
		
		filename = "$profile:Editor/Export" + filename;
		
		
		DeleteFile(filename);
		FileHandle handle = OpenFile(filename, FileMode.WRITE | FileMode.APPEND);
		if (handle == 0) {
			Print("ExportToFile Failed: 0");
			return;
		}
		
		foreach (EditorObject editor_object: export_objects) {
						
			vector position = editor_object.GetPosition();
			vector orientation = editor_object.GetOrientation();
			orientation = orientation.VectorToAngles();
			float scale = 1;//editor_object.GetScale();
	
			vector terrainbuilder_offset = Vector(200000, 0, 0);
			string line;
			switch (mode) {
				
				case ExportMode.TERRAINBUILDER: {
					// "construction_house2";206638.935547;6076.024414;146.000015;0.000000;0.000000;1.000000;
					// Name, X, Y, Yaw, Pitch, Roll, Scale, Relative Height
					array<LOD> testlods = new array<LOD>();
					editor_object.GetObject().GetLODS(testlods);
					
					foreach (LOD lod: testlods) {
						Print(editor_object.GetObject().GetLODName(lod));
						array<Selection> selections = new array<Selection>();
						lod.GetSelections(selections);
						foreach (Selection s: selections) {
							
							Print(s.GetName());
							for (int fff = 0; fff < s.GetVertexCount(); fff++) {
								Print(s.GetVertexPosition(lod, fff));
							}
						}
					}
					
					//if (height_type == HeightType.RELATIVE)
						//position[1] = position[1] - GetGame().SurfaceY(position[0], position[2]);
					
					position = position + terrainbuilder_offset;
					line = string.Format("\"%1\";%2;%3;%4;%5;%6;%7;%8;", editor_object.GetModelName(), position[0], position[2], orientation[0], orientation[1], orientation[2], scale, position[1]);
					FPrintln(handle, line);
					break;
				}
					
				case ExportMode.COMFILE: {
					// SpawnObject("Land_Construction_House2", "6638.935547 7.190318 6076.024414", "146.000015 0.000000 0.000000")
					line = string.Format("SpawnObject(\"%1\", \"%2\", \"%3\");", editor_object.GetType(), position.ToString(false), orientation.ToString(false));
					FPrintln(handle, line);
					break;
				}
				
				case ExportMode.EXPANSION: {
					// Land_Construction_House2|13108.842773 10.015385 6931.083984|-101.999985 0.000000 0.000000
					//orientation = orientation.VectorToAngles();
					line = string.Format("%1|%2 %3 %4|%5 %6 %7", editor_object.GetType(), position[0], position[1], position[2], orientation[0], orientation[1], orientation[2]);
					FPrintln(handle, line);
					break;
				}
				
				case ExportMode.VPP: {
					
					//line = string.Format("%1|%2 %3 %4|%5 %6 %7", editor_object.GetType(), position[0], position[1], position[2], orientation[0], orientation[1], orientation[2]);
					//FPrintln(handle, line);
					break;
				}
				
				default: {
					FPrintln(handle, "Line Export Failure");
					break;
				}
				
				
				
			} 
		}
		
		CloseFile(handle);
		
		GetEditor().GetUIManager().NotificationCreate("Exported!");
		
	}
}