/*Copyright (c) 2021, Autodesk Inc 
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this file ("RenderTimeInstancing.h") and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//headers from Max SDK
#include <object.h>
#include <mesh.h>
#include <containers/array.h>

#define RENDERTIME_INSTANCING_INTERFACE Interface_ID(0x442741c3, 0x2e22675c)

class Mtl;

/*
    The RenderTimeInstancing interface allows you to access instancing information for an object
    at render time, so that a renderer can do efficient instancing of source objects.

    This interface is implemented by an object, and called by a renderer. 
    
    NOTE: It is legal for a renderer to iterate these loops from MULTIPLE THREADS. However,
          the *same* thread should not concurrently access multiple RenderInstance's at the
          same literal time. This makes it legal for the object plugin to actually reuse
          the memory returned from GetRenderInstance() to improve performance, as long
          as that memory block is kept separete PER THREAD.

    Do not call GetRenderMesh() for an object that supports this interface. For an object 
    that does IMPLEMENT this interface, GetRenderMesh() should return an aggregate mesh
    of all instances, such that a renderer that does NOT support this interface at least will
    render *something*

    An instance may have custom data channels. 

    USAGE:

    RenderTimeInstancing* instancer = GetRenderTimeInstancing(baseObject);

    if (instancer)
    {	
        //UpdateInstanceData

        Interval valid = FOREVER;
        instancer->UpdateInstanceData(t, valid, _T("myPlugin"));
        
        //To ensure maximum data access speed, we convert our channel
        //strings into channel indices outside of the instance loop 

        //Channel strings are arbitrary and defined by the object.
        //Safety checks should be in place to ensure attempts to access 
        //a missing channel will not cause any errors. 
        //A default value for missing channels will simply
        //be returned instead (0.0f, Point3::Origin, Matrix3(1))

        // Note: channel names are case-sensitive

        int floatChannel1  = instancer->FloatChannelToInt(_T("myFloatChannel")); 
        int VectorChannel1 = instancer->VectorChannelToInt(_T("myVectorChannel1"));
        int VectorChannel2 = instancer->VectorChannelToInt(_T("myVectorChannel2"));
        int TMChannel1     = instancer->TMChannelToInt(_T("myTMChannel"));

        // Instancer acts as a container of sources. Loop over the sources in the instancer
        for (auto source : instancer)
            auto flags = source->GetFlags();

            if (flags & DataFlags::mesh)  { ... work with meshes .... } 
            if (flags & DataFlags::inode) { ... work with iNode .... }

            // Get what to be instanced
            void *data = source->GetData();

            // A source acts as a container of targets
            for (auto target : source )
            {
                float   f1  = target->GetCustomFloat(floatChannel1);
                Point3  v1  = target->GetCustomVector(vectorChannel1);
                Point3  v2  = target->GetCustomVector(vectorChannel2);
                Matrix3 tm1 = target->GetCustomVector(TMChannel1);

                // ... actually instance the "source" object using info in "target"

                ... renderer specific magic goes here....

            }

            if (flags & DataFlags::mesh &&  flags DataFlags::pluginMustDelete)
                      ... delete the mesh ....

        }
        // Cleanup, if any is needed
        instancer->ReleaseInstanceData();
    }
    else
        ...use GetRenderMesh() instead....
*/
    
namespace RenderTimeInstancing
{
    struct InstanceUVWInfo { int channel; UVVert value; };

    enum DataFlags : signed int
    {
        none = 0,

        mesh = 1 << 0, //RenderInstanceSource::data* is Mesh*
        inode = 1 << 1, //RenderInstanceSource::data* is INode*

        pluginMustDelete = 1 << 31 //set in RenderInstanceSource::flags if 
        //the calling plugin must delete data pointer after use
    };



    class RenderInstanceSource;


    class RenderInstance
    {
        /*
        These functions return custom data values for each
        instance. Values are retreived using channel integers
        */
        virtual float   GetCustomFloat(int channelInt) = 0;
        virtual Point3  GetCustomVector(int channelInt) = 0;
        virtual Matrix3 GetCustomTM(int channelInt) = 0;


        // ZAP: What is this, do we need this? (Originally GetParticleExportGroups). Took it out for now
                /*
                This function returns per-instance export group flags
                A return value of 0 means no flags have been set.
                */
                //      virtual unsigned int GetExportGroups() = 0;

                        /*ID contains the unique Birth ID of source instances (i.e. Birth ID
                        of a particle, or scattered item). This value should be unique for each
                        instance in the set. This value can be negative or zero*/
        virtual __int64 GetID();

        /*instanceID contains the arbitrary, user-defined instance ID of source
        instances. Texmaps can make use of this value at rendertime. This value
        can be negative or zero. */
        virtual __int64 GetInstanceID();


        // ZAP: What is this, do we need this? Took it out for now
                /*
                This function returns per-instance instanceNode. This is a user-defined
                render-only node which corresponds to each instance. NULL means no
                node has been assigned.
                */
                //      virtual INode* GetParticleInstanceNode() = 0;


                        /*
                        This function returns per-instance mesh matID overrides.
                        A return value of -1 means no override is set on the instance.
                        */
        virtual int GetMatID() = 0;

        /*
        This function returns per-instance material (Mtl*) overrides.
        A return value of NULL means no override is set on the instance and
        thus the default node material should be used.
        */
        virtual Mtl* GetMtl() = 0;

        // ZAP: What is this, do we need this? (GetParticleSimGroupsByIndex). Took out for now
                /*
                This function returns per-instance simulation group flags
                A return value of 0 means no flags have been set.
                */
                //      virtual unsigned int GetSimGroups() = 0;


                        /*
                        This function returns per-instance UVW overrides for specific map
                        channels.
                        The return value is an array which contains a list of overrides
                        and the map channel whose vertices they should be assigned to. An
                        empty array means no UVW overrides have been assigned to the instance.
                        */
        virtual MaxSDK::Array<InstanceUVWInfo> GetUVWsVec() = 0;


        /*tms contains the instance's transform(s) spread evenly over the motion
        blur interval (the interval specified by the arguments passed to
        CollectInstances), in temporal order. A tms tyVector with a single element
        represents a static instance. A tms tyVector with two elements contains
        the transforms at the start and end of the interval. A tms tyVector with
        three elements contains the transforms at the start, center, and end of
        the interval, etc. A tms tyVector with more than two elements allows
        a renderer to compute more accurate multi-sample motion blur.

        Instance velocity/spin, should those properties be required, should be
        derived from these values (typically from the first/last entry)*/
        virtual MaxSDK::Array<Matrix3> GetTMs();


        // ZAP: Do we need these?
        //      and doesn't the spin need to be a quaternion to even work??
        // 
                /*vel is the per-frame instance velocity of the instance. Note: this value
                is stored for completeness, but should not be used by developers to calculate
                motion blur. Motion blur should be calculated using the tms tyVector instead.
                */
        virtual Point3 GetVelocity();

        /*spin is the per-frame instance spin of the instance. Note: this value
        is stored for completeness, but should not be used by developers to calculate
        motion blur. Motion blur should be calculated using tm0 and tm1 instead.
        */
        virtual Quat GetSpin();
    };

    class RenderInstanceSource
    {
        /* INFO ABOUT THE ACTUAL THING BEING INSTANCED */

        /*these flags are used to define the type of data stored
        in the void pointer, and any other relevant information,
        like whether the plugin must delete the pointer once it's
        finished using it.
        */
        virtual DataFlags GetFlags();

        /*the data pointer contains the relevant class that should be
        instanced. Currently, it is either a Mesh* or an INode*, and
        the flags variable can be queried to find out which class type it is.

        The flags variable will only have one class type flag set, but may
        have other relevant information flagged as well, so you should not
        test for the class type with the equality ('==') operator, but instead
        bitwise AND operator ('&'). For example:

        if (flags == DataFlags::mesh){auto mesh = (Mesh*)data;} //incorrect
        if (flags == DataFlags::inode){auto node = (INode*)data;} //incorrect

        if (flags & DataFlags::mesh){auto mesh = (Mesh*)data;} //correct
        if (flags & DataFlags::inode){auto node = (INode*)data;} //correct

        If the "pluginMustDelete" flag is set, you must delete this pointer
        after use. Be sure to cast to relevant class before deletion
        so the proper destructor is called.
        */
        virtual void *GetData();

        /*
        This function returns the map channel where per-vertex
        velocity data (stored in units/frame) might be found, inside
        any meshes returned by the RenderTimeInstancing.

        A value of -1 means the mesh contains no per-vertex velocity data.

        Note: not all meshes are guaranteed to contain velocity data. It is
        your duty to check that this map channel is initialized on a given
        mesh and that its face count is equal to the mesh's face count.
        If both face counts are equal, you can retrieve vertex velocities
        by iterating each mesh face's vertices, and applying the
        corresponding map face vertex value to the vertex velocity array
        you are constructing. Vertex velocities must be indirectly retrieved
        by iterating through the faces like this, because even if the map
        vertex count is identical to the mesh vertex count, the map/mesh
        vertex indices may not correspond to each other.

        Here is an example of how vertex velocities could be retrieved from
        the velocity map channel, through a RenderTimeInstancing:

        ////

        std::vector<Point3> vertexVelocities(mesh.numVerts, Point3(0,0,0));

        int velMapChan = RenderTimeInstancing->GetMeshVelocityMapChannel();
        if (velMapChan >= 0 && mesh.mapSupport(velMapChan))
        {
            MeshMap &map = mesh.maps[velMapChan];
            if (map.fnum == mesh.numFaces)
            {
                for (int f = 0; f < mesh.numFaces; f++)
                {
                    Face &meshFace = mesh.faces[f];
                    TVFace &mapFace = map.tf[f];

                    for (int v = 0; v < 3; v++)
                    {
                        int meshVInx = meshFace.v[v];
                        int mapVInx = mapFace.t[v];
                        Point3 vel = map.tv[mapVInx];
                        vertexVelocities[meshVInx] = vel;
                    }
                }
            }
        }

        */

        virtual int GetVelocityMapChannel();

        /* ACCESS TO THE ACTUAL INSTANCES */

        /* Get the number of instances of this kind, and the actual instances. */
        virtual size_t          GetNumInstances();
        virtual RenderInstance *GetRenderInstance(size_t index);

        // For convenicence - iterator
        class Iterator;
        Iterator begin() { return Iterator(this); }
        Iterator end()   { return Iterator(this, GetNumInstances()); }
        class Iterator {
        public:
            Iterator(RenderInstanceSource *item) : m_item(item), m_i(0) {}
            Iterator(RenderInstanceSource *item, const size_t val) : m_item(item), m_i(val) {}

            Iterator&             operator++() { m_i++; return *this; }
            bool                  operator!=(const Iterator &iterator) { return m_i != iterator.m_i; }
            const RenderInstance *operator*() { return m_item->GetRenderInstance(m_i); }
        private:
            size_t                 m_i;
            RenderInstanceSource  *m_item;
        };
    };

    class RenderTimeInstancing {
    public:
        /* SETUP / UPDATING */

        /*
        This needs to be called to make sure the instance data is ready to
        be retreived, and makes sure everything is up-to date to be read.

        The plugin argument of this function takes the name of the plugin
        querying this interface, in lowercase letters.
        Ex: _T("arnold"), _T("octane"), _T("redshift"), _T("vray"), etc.
        This is a somewhat arbitrary value, but by having plugins identify
        themselves during a query, tyFlow can internally determine if any
        plugin-specific edge-cases need to be processed.
        */
        virtual void UpdateInstanceData(TimeValue t, Interval &valid, TSTR plugin) = 0;

        /*
        When a caller of this interface is done with the data, it can call ReleseInstanceData().
        In case the generating plugin allocated some information, it can be released here.
        */
        virtual void ReleaseInstanceData() = 0;


        /* INSTANCE DATA ACCESS */

        /*
        These functions return a list of active channel names for
        each data type
        */
        virtual MaxSDK::Array<TSTR> GetFloatChannelNamesVec() = 0;
        virtual MaxSDK::Array<TSTR> GetVectorChannelNamesVec() = 0;
        virtual MaxSDK::Array<TSTR> GetTMChannelNamesVec() = 0;

        /*
        These functions convert channel strings into channel integers
        */
        virtual int FloatChannelToInt(TSTR channel) = 0;
        virtual int VectorChannelToInt(TSTR channel) = 0;
        virtual int TMChannelToInt(TSTR channel) = 0;

        /* Getting the actual things TO BE instanced */
        virtual size_t                 GetNumInstanceSources();
        virtual RenderInstanceSource  *GetRenderInstanceSource(size_t index);

        // For convenicence - iterator
        class Iterator;
        Iterator begin() { return Iterator(this); }
        Iterator end()   { return Iterator(this, GetNumInstanceSources()); }
        class Iterator {
        public:
            Iterator(RenderTimeInstancing *item) : m_item(item), m_i(0) {}
            Iterator(RenderTimeInstancing *item, const size_t val) : m_item(item), m_i(val) {}

            Iterator&             operator++() { m_i++; return *this; }
            bool                  operator!=(const Iterator &iterator) { return m_i != iterator.m_i; }
            RenderInstanceSource *operator*() { return m_item->GetRenderInstanceSource(m_i); }
        private:
            size_t                 m_i;
            RenderTimeInstancing  *m_item;
        };
    };

    inline RenderTimeInstancing* GetRenderTimeInstancing(BaseObject* obj)
    {
        return (RenderTimeInstancing*)obj->GetInterface(RENDERTIME_INSTANCING_INTERFACE);
    }
}