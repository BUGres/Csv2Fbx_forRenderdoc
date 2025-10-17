#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <fbxsdk.h>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

// 定义顶点结构
struct Vertex {
    int index; // 控制点索引
    FbxVector4 position; // 位置
    FbxVector4 normal; // 法线
    FbxVector2 uv; // UV
};

// 创建FBX文件
void CreateFbxFile(const std::string& filename, const std::vector<Vertex>& vertices) {
    // 初始化FBX SDK
    FbxManager* lSdkManager = FbxManager::Create();
    if (!lSdkManager) {
        std::cerr << "无法创建FBX SDK管理器" << std::endl;
        return;
    }

    // 创建IO设置
    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    // 创建场景
    FbxScene* lScene = FbxScene::Create(lSdkManager, "MyScene");

    // 创建网格
    FbxMesh* lMesh = FbxMesh::Create(lScene, "MyMesh");

    // 初始化控制点
    int numVertices = vertices.size();
    lMesh->InitControlPoints(numVertices);

    // 设置控制点位置
    FbxVector4* lControlPoints = lMesh->GetControlPoints();
    for (size_t i = 0; i < numVertices; ++i) {
        lControlPoints[i] = vertices[i].position;
    }

    // 添加多边形
    for (size_t i = 0; i + 2 < numVertices; i += 3) {
        lMesh->BeginPolygon(-1, -1, false);
        lMesh->AddPolygon(i);
        lMesh->AddPolygon(i + 1);
        lMesh->AddPolygon(i + 2);
        lMesh->EndPolygon();
    }

    // 添加法线
    FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
    lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
    lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);

    for (const auto& vertex : vertices) {
        lNormalElement->GetDirectArray().Add(vertex.normal);
    }

    // 添加UV
    FbxGeometryElementUV* lUVElement = lMesh->CreateElementUV("defaultUV");
    lUVElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
    lUVElement->SetReferenceMode(FbxGeometryElement::eDirect);

    for (const auto& vertex : vertices) {
        lUVElement->GetDirectArray().Add(vertex.uv);
    }

    // 创建节点并将网格添加到节点中
    FbxNode* node = FbxNode::Create(lScene, "MyMeshNode");
    node->SetNodeAttribute(lMesh);
    lScene->GetRootNode()->AddChild(node);

    // 保存场景到文件
    FbxExporter* lExporter = FbxExporter::Create(lSdkManager, "");
    if (!lExporter->Initialize("tmp", -1, lSdkManager->GetIOSettings())) {
        std::cerr << "无法初始化FBX导出器: " << lExporter->GetStatus().GetErrorString() << std::endl;
        return;
    }

    lExporter->Export(lScene);
    lExporter->Destroy();

    // 销毁场景和管理器
    lScene->Destroy();
    lSdkManager->Destroy();

    std::remove(filename.c_str());
    std::rename("tmp.fbx", filename.c_str());
}

std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }

        data.push_back(row);
    }

    return data;
}

int main(int argc, char* argv[])
{
    //std::vector<Vertex> vertices = {
    //{0, FbxVector4(-1.0f, -1.0f, 0.0f), FbxVector4(0.0f, 0.0f, 1.0f), FbxVector2(0.0f, 0.0f)},
    //{1, FbxVector4(1.0f, -1.0f, 0.0f), FbxVector4(0.0f, 0.0f, 1.0f), FbxVector2(1.0f, 0.0f)},
    //{2, FbxVector4(0.0f, 1.0f, 0.0f), FbxVector4(0.0f, 0.0f, 1.0f), FbxVector2(0.5f, 1.0f)}
    //};
    //CreateFbxFile("test", vertices);
    //return 0;

    cout << "将处理从RenderDoc导出的CSV文件（放在本程序同一文件夹下），将其转化为模型，请输入转换模式（a~z小写字母）并按下回车" << endl;
    cout << "将自动检查POSITION NORMAL TEXCOORD => 位置 法线 UV，请注意顺序问题" << endl;

    // 获取当前工作目录
    fs::path current_path = fs::current_path();
    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(current_path)) {
        // 检查文件扩展名是否为 .csv
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            string csvFileName = entry.path().filename().string();
            std::cout << "  " << ++fileCount << " " << csvFileName << std::endl;

            auto csvFile = readCSV(csvFileName);

            int controlIndex = 1;
            int posIndex = -1;
            int normalIndex = -1;
            int uvIndex = -1;

            cout << "表格行数" << csvFile.size() << " 表格列数" << csvFile[0].size() << endl;
            for (int i = 0; i < csvFile[0].size(); i++)
            {
                if (csvFile[0][i] == " POSITION.x")
                {
                    cout << "Find! " << csvFile[0][i] << " index = " << i << endl;
                    posIndex = i;
                }
                else if (csvFile[0][i] == " NORMAL.x")
                {
                    cout << "Find! " << csvFile[0][i] << " index = " << i << endl;
                    normalIndex = i;
                }
                else if (csvFile[0][i] == " TEXCOORD0.x")
                {
                    cout << "Find! " << csvFile[0][i] << " index = " << i << endl;
                    uvIndex = i;
                }
            }

            if (posIndex == -1 || normalIndex == -1 || uvIndex == -1)
            {
                cout << "没能全部找到POSITION NORMAL TEXCOORD，CSV的列名称中似乎没有\" POSITION.x\" \" NORMAL.x\" \" TEXCOORD0.x\"" << endl;
                system("pause");
                return 0;
            }
            else
            {
                cout << "index control pos normal uv => 2 " << posIndex << " " << normalIndex << " " << uvIndex << endl;
            }

            // 下面生成fbx
            std::vector<Vertex> vstream;
            for (int i = 1; i < csvFile.size(); i++)
            {
                // cout << i << endl;
                
                Vertex v;
                v.index = std::stof(csvFile[i][controlIndex]);
                v.position = FbxVector4(std::stof(csvFile[i][posIndex]) * 100, 
                    std::stof(csvFile[i][posIndex + 1]) * 100,
                    std::stof(csvFile[i][posIndex + 2]) * 100,
                    0);
                v.normal = FbxVector4(std::stof(csvFile[i][normalIndex]),
                    std::stof(csvFile[i][normalIndex + 1]),
                    std::stof(csvFile[i][normalIndex + 2]),
                    0);
                v.uv = FbxVector2(std::stof(csvFile[i][uvIndex]),
                    std::stof(csvFile[i][uvIndex + 1]));
                vstream.push_back(v);
            }

            CreateFbxFile(csvFileName.substr(0, csvFileName.size() - 4) + ".fbx", vstream);
        }
    }

    system("pause");
    return 0;
}
