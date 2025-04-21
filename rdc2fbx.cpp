#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <fbxsdk.h>
#include <cstdlib>

using namespace std;

int POS = 0;
int UV = 0;

// 定义一个结构体来存储三角形的顶点和UV坐标
struct TriangleData
{
    FbxVector4 vertices[3]; // 顶点坐标
    FbxVector2 uv[3]; // UV坐标
};

// 手动生成三角形数据
void GenerateTriangles(std::vector<TriangleData>& triangles)
{
    // 打开文件
    auto filename = "model.txt";
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // 逐行读取文件内容
    std::string line;
    int vcount = 0;
    float* vertex0 = new float[50];
    float* vertex1 = new float[50];
    while (std::getline(file, line))
    {
        float* list = new float[50];
        int listIndex = 0;
        int last = 0;
        for (int i = 0; i < line.length(); i++)
        {
            if (line[i] == '\t' || line[i] == '\n' || i == line.length() - 1)
            {
                string substr = line.substr(last, i - last);
                if (i == line.length() - 1)substr = line.substr(last, i - last + 1);
                list[listIndex++] = 100 * std::atof(substr.c_str());
                last = i + 1;
            }
        }

        if (vcount % 3 == 2)
        {
            TriangleData* t = new TriangleData();

            t->vertices[0] = FbxVector4(vertex0[POS], vertex0[POS + 1], vertex0[POS + 2]); // 顶点0
            t->vertices[1] = FbxVector4(vertex1[POS], vertex1[POS + 1], vertex1[POS + 2]);  // 顶点1
            t->vertices[2] = FbxVector4(list[POS], list[POS + 1], list[POS + 2]);  // 顶点2

            t->uv[0] = FbxVector2(vertex0[UV] / 100, vertex0[UV + 1] / 100); // UV0
            t->uv[1] = FbxVector2(vertex1[UV] / 100, vertex1[UV + 1] / 100); // UV1
            t->uv[2] = FbxVector2(list[UV] / 100, list[UV + 1] / 100); // UV2

            triangles.push_back(*t);
        }
        else if (vcount % 3 == 1)
        {
            vertex1 = list;
        }
        else
        {
            vertex0 = list;
        }

        vcount++;
        if (vcount % 10000 == 0) std::cout << vcount << " vertex" << endl;
    }
}

// 创建FBX网格并添加到场景中
void CreateFbxMesh(FbxScene* scene, const std::vector<TriangleData>& triangles)
{
    // 创建一个网格节点
    FbxNode* node = FbxNode::Create(scene, "MyMeshNode");
    FbxMesh* mesh = FbxMesh::Create(scene, "MyMesh");
    node->SetNodeAttribute(mesh);
    scene->GetRootNode()->AddChild(node);

    // 初始化控制点数组
    int totalVertices = 0;
    for (const auto& triangle : triangles)
    {
        totalVertices += 3;
    }
    mesh->InitControlPoints(totalVertices);
    FbxVector4* controlPoints = mesh->GetControlPoints();

    // 添加控制点
    int vertexIndex = 0;
    for (const auto& triangle : triangles)
    {
        for (int i = 0; i < 3; ++i)
        {
            controlPoints[vertexIndex++] = triangle.vertices[i];
        }
    }

    // 添加多边形（三角形）
    int indices = 0;
    for (const auto& triangle : triangles)
    {
        mesh->BeginPolygon();
        for (int i = 0; i < 3; ++i)
        {
            mesh->AddPolygon(indices++);
        }
        mesh->EndPolygon();
    }

    // 添加UV层
    FbxLayer* layer = mesh->GetLayer(0);
    if (!layer)
    {
        mesh->CreateLayer();
        layer = mesh->GetLayer(0);
    }

    FbxLayerElementUV* uvLayer = FbxLayerElementUV::Create(mesh, "UV");
    uvLayer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
    uvLayer->SetReferenceMode(FbxLayerElement::eDirect);

    int polygonCount = mesh->GetPolygonCount();
    int polygonVertexIndex = 0;
    for (int i = 0; i < polygonCount; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            uvLayer->GetDirectArray().Add(triangles[i].uv[j]);
        }
    }

    layer->SetUVs(uvLayer);
}

int StringToInt(const std::string& str)
{
    try
    {
        return std::stoi(str);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        return -1;
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Out of range: " << e.what() << std::endl;
        return -1;
    }
}

int main(int argc, char* argv[])
{
    POS = StringToInt(argv[1]);
    UV = StringToInt(argv[2]);

    // 初始化FBX SDK管理器
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ioSettings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ioSettings);

    // 创建一个空的FBX场景
    FbxScene* scene = FbxScene::Create(manager, "MyScene");

    // 手动生成三角形数据
    std::vector<TriangleData> triangles;
    GenerateTriangles(triangles);

    // 将三角形数据添加到FBX场景中
    CreateFbxMesh(scene, triangles);

    // 保存FBX文件
    FbxExporter* exporter = FbxExporter::Create(manager, "");
    if (!exporter->Initialize("output.fbx", -1, manager->GetIOSettings()))
    {
        std::cerr << "Failed to initialize exporter." << std::endl;
        return -1;
    }

    if (!exporter->Export(scene))
    {
        std::cerr << "Failed to export scene." << std::endl;
        return -1;
    }

    exporter->Destroy();

    // 清理
    scene->Destroy();
    manager->Destroy();

    //std::cout << "FBX file saved successfully." << std::endl;
    //std::string end;
    //cin >> end;
    return 0;
}
