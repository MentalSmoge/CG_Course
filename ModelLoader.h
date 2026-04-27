#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "GameComponent.h"
#include <WICTextureLoader.h>
#include <string>
class ModelLoader {
public:
    GameComponent::MeshData LoadModel(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& path)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace | aiProcess_FlipUVs
        );

        aiMesh* mesh = scene->mMeshes[0];
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto texture = LoadEmbeddedTexture(device, context, scene, material);

        GameComponent::MeshData result;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            GameComponent::Vertex v;

            v.pos = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z,
                1
            };

            v.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };

            v.color = { 1,1,1,1 };

            if (mesh->mTextureCoords[0])
            {
                v.uv = {
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                };
            }
            else
            {
                v.uv = { 0,0 };
            }

            result.vertices.push_back(v);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                result.indices.push_back(face.mIndices[j]);
            }
        }
        result.texture = texture;

        return result;
    }
    ID3D11ShaderResourceView* LoadEmbeddedTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const aiScene* scene,
        aiMaterial* material
    )
    {
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) != AI_SUCCESS)
            return nullptr;

        const aiTexture* tex = scene->GetEmbeddedTexture(path.C_Str());
        if (!tex)
            return nullptr;

        ID3D11ShaderResourceView* srv = nullptr;

        if (tex->mHeight == 0)
        {
            CreateWICTextureFromMemory(
                device,
                context,
                reinterpret_cast<const uint8_t*>(tex->pcData),
                tex->mWidth,
                nullptr,
                &srv
            );
        }
        else
        {

        }

        return srv;
    }
};