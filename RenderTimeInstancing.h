/*Copyright (c) 2021, Autodesk Inc, All Rights Reserved.

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
    
namespace MaxSDK
{
    namespace RenderTimeInstancing
    {
        class  RenderInstanceSource;
        struct MotionBlurInfo;

        /*! \brief The RenderTimeInstancing interface allows you to access instancing information for an object
                  render time, so that a renderer can do efficient instancing of one or more source objects 
                  at render time. This interface is implemented by an object, and called by a renderer.

            \note It is legal for a renderer to iterate these loops from <em>multiple threads</em>.
                  However, the <em>same thread</em> should not concurrently access multiple RenderInstanceTarget's at the
                  same literal time. This makes it legal for the object plugin to actually reuse
                  the memory returned from GetRenderInstanceTarget() to improve performance, as long
                  as that memory block is kept separete <em>per thread</em>.

            A renderer should not call GetRenderMesh() for an object that supports this interface. For an object
            that <em>implements</em> this interface, GetRenderMesh() should ideally be implemented and return an aggregate
            mesh of all instances, such that a renderer that does <em>not</em> support this interface at least will
            render <em>something</em>.

            An instance may have custom data channels. The data channel names used by an instancer can be retreived
            with the functions GetFloatChannelNames(), GetVectorChannelNames() or GetTMChannelNames().

            Channel names are then mapped to integer tokens using FloatChannelToInt(), VectorChannelToInt() and
            TMChannelToInt(). The actual values for each instance are then retreived using these integer tokens.

            Usage example:
            \code
            RenderTimeInstancing* instancer = RenderTimeInstancing::GetRenderTimeInstancing(baseObject);

            if (instancer)
            {
                //UpdateInstanceData

                Interval valid = FOREVER;

                // Initialize motion blur info, which is a two way communication
                // between renderer and the object. Renderer fills in its desires
                // and object responds with what it actually can accomodate
                MotionBlurInfo mblur(Interval(shutterOpen, shutterClose));

                instancer->UpdateInstanceData(t, valid, mblur, _T("myPlugin"));

                // Get integers for known channel names

                int floatChannel1  = instancer->FloatChannelToInt(_T("myFloatChannel"));
                int VectorChannel1 = instancer->VectorChannelToInt(_T("myVectorChannel1"));
                int VectorChannel2 = instancer->VectorChannelToInt(_T("myVectorChannel2"));
                int TMChannel1     = instancer->TMChannelToInt(_T("myTMChannel"));

                // Instancer acts as a container of sources. Loop over the sources in the instancer
                for (auto source : *instancer)
                    auto flags = source->GetFlags();

                    if (flags & DataFlags::mesh)  { ... work with meshes .... }
                    if (flags & DataFlags::inode) { ... work with iNode .... }

                    // Get what to be instanced
                    void *data = source->GetData();

                    // A source acts as a container of targets
                    for (auto target : *source )
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
            \endcode
        */
        class RenderTimeInstancing {
        public:
            /*! \name Setup, Update and Release */
            ///@{

            /*! \brief Make sure instancing data is up-to-date

            This needs to be called to make sure the instance data is ready to
            be retreived, and makes sure everything is up-to date to be read.

            @param t The time of the evaluation. Most often same as the shutter open time, 
                     if motion blur is used.

            @param valid Returns the validity of the returned data. For example, if FOREVER is 
                         returned it indicates to the renderer that the instances are not moving
                         or changing at all, and could in principle be retained over multiple frames.

            @param mbinfo 
                accepts an initialized MotionBlurInfo, which upon
                return will contain info on how the motion data is returned.

            @param plugin
                The plugin argument of this function takes the name of the plugin
                querying this interface, in lowercase letters.
                Ex: _T("arnold"), _T("octane"), _T("redshift"), _T("vray"), etc.                
                This is a somewhat arbitrary value, but by having renderers identify
                themselves during a query, the object can internally determine if any
                renderer-specific edge-cases need to be processed.
            */
            virtual void UpdateInstanceData(TimeValue t, Interval &valid, MotionBlurInfo &mbinfo, TSTR plugin) = 0;

            /*! \brief Release the instancing data.

            When a caller of this interface is done with the data, it can call ReleseInstanceData().
            In case the generating plugin allocated some information, it can be released here.
            */
            virtual void ReleaseInstanceData() = 0;
            ///@}

            /*! \name Getting Data Channel Names
                Retreives the names of data channels for each supported type  */
            ///@{
            //! \brief Return a list of active channel names for float channels
            virtual MaxSDK::Array<TSTR> GetFloatChannelNames() = 0;
            //! \brief Return a list of active channel names for vector channels
            virtual MaxSDK::Array<TSTR> GetVectorChannelNames() = 0;
            //! \brief Return a list of active channel names for TM channels
            virtual MaxSDK::Array<TSTR> GetTMChannelNames() = 0;
            ///@}

            /*! \name Data Channel Integer Access
                These functions convert channel strings into channel integers.
                Channel strings are arbitrary and defined by the object.

                Safety checks should be in place to ensure attempts to access
                a missing channel will not cause any errors.
                A default value for missing channels will simply
                be returned instead (0.0f, Point3::Origin, Matrix3(1))

                \note Channel names are case-sensitive
            */
            ///@{
            /*! \brief Get integer index for a float channel name */
            virtual int FloatChannelToInt(TSTR channel) = 0;
            /*! \brief Get integer index for a vector channel name */
            virtual int VectorChannelToInt(TSTR channel) = 0;
            /*! \brief Get integer index for a TM channel name */
            virtual int TMChannelToInt(TSTR channel) = 0;
            ///@}

            /*! \name Getting the actual things to be instanced (the sources) */
            ///@{
            /*! \brief Get number of sources */
            virtual size_t                 GetNumInstanceSources() = 0;
            /*! \brief Get the n:th source */
            virtual RenderInstanceSource  *GetRenderInstanceSource(size_t index) = 0;

            //! \brief For convenicence - iterator
            class Iterator;
            //! \brief Retreive the begin() iterator. Allows using a for (auto x : y) loop
            Iterator begin() { return Iterator(this); }
            //! \brief Retreive the end() iterator
            Iterator end() { return Iterator(this, GetNumInstanceSources()); }
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
            ///@}
        };

        /*! \brief Defines MotionBlurInfo::flags means */
        enum MBFlags : signed int
        {
            transforms    = 1 << 0,  //!< \brief Motion blur is communicated via RenderInstanceTarget::GetTM() returning multiple matrices
            velocities    = 1 << 1,  //!< \brief Motion blur is communicated via RenderInstanceTarget::GetVelocity() and RenderInstanceTarget::GetSpin()
            tm_preferred  = 1 << 2,  //!< \brief The transformations hold the most "true" data. If velocities are returned, they are computed from them
            vel_preferred = 1 << 3,  //!< \brief The velocities hold the most "true" data. If TM's are returned, they are computed from them
        };

        /*! \brief Motion Blur information struct.

        This communicates information about shutter intervals and motion blur behavior between the object and the renderer.
        
        It is filled in and passed RenterTimeInstancing::UpdateInstanceData() by the renderer and the object may modify it
        to communicate back to it. 
        */
        struct MotionBlurInfo {
            /*! \brief Defines what kind of motion is desired and used
            \see RenderTimeInstancing::MBFlags */
            MBFlags   flags;            
            /*! \brief Defines the open and closing time of the shutter.
            If set to NEVER, motion blur is not used. */
            Interval  shutterInterval;  
            /*! \brief The desired number of TMs returned by GetTMs(), or -1 for variable.

            Here, the renderer can request a desired number of tranformations returned by GetTM(), if
            the renderer only supports to have a fixed set. 
            
            When this is passed into the RenterTimeInstancing::UpdateInstanceData() function from the renderer, the meaning is as follows

            - If this value is -1 (the default) it means "I don't care", and there can be a variable number of TM's returned.
            - If this value is 0, it means the renderer is not rendering with motion blur and will not care
            - If this value is 1, it indicates the renderer expects a single tranform, and any motion to be handed over in GetVelocity() and GetSpin()
            - A value of 2 or greater means that the renderer expects exactly that many TMs returned from GetTMs(), no more, no less.

            Upon returning from the RenterTimeInstancing::UpdateInstanceData() call, the object can fill in the response value

            - If this value is -1 it means there might be a variable number of TM's returned from GetTMs().
            - If this value is 0, it means the instances are not moving at all, no motion blur needs to be calculated due to instance motion (there might still be vertex motion though!)
            - If this value is 1, it means only a single TM is returned. Any motion will be in GetVelocity() and GetSpin() only.
            - A value of 2 or greater means that many TMs will always be returned, no more, no less.

            */
            int       numberOfTMs;       

            MotionBlurInfo(Interval shutter = NEVER, MBFlags f = MBFlags::tm_preferred, int tms = -1) { 
                shutterInterval = shutter;
                flags           = f;
                numberOfTMs     = tms;
            };
        };

        /*! \brief UVW channel override data */
        struct InstanceUVWInfo { int channel; UVVert value; };

        /*! \brief Defines what GetData() returns and how to treat it */
        enum DataFlags : signed int
        {
            none  = 0,

            mesh  = 1 << 0, //!< \brief RenderInstanceSource::GetData() is Mesh*
            inode = 1 << 1, //!< \brief RenderInstanceSource::GetData() is INode*

            pluginMustDelete = 1 << 31 //!< \brief Set if the renderer is expected to delete the data pointer after use
        };

        /*! \brief Information about a given instance of a RenderInstanceSource */
        class RenderInstanceTarget
        {
        public:

            /*! \name Instance Custom Data Access
            
            These functions return custom data values for each
            instance. Values are retreived using channel integers
            */
            ///@{
            virtual float   GetCustomFloat (int channelInt) = 0;  //!< \brief Return a float value
            virtual Point3  GetCustomVector(int channelInt) = 0;  //!< \brief Return a vector value
            virtual Matrix3 GetCustomTM    (int channelInt) = 0;  //!< \brief Return a TM value
            ///@}


            /*! \name Instance Standard Data Access

            These functions return the standard set of data for an instance.
            */
            ///@{
            /*! \brief Get unique instance ID.

                GetID() returns the unique Birth ID of source instances (i.e. Birth ID
                of a particle, or scattered item). This value should be unique for each
                instance in the set. This value can be negative or zero*/
            virtual __int64 GetID() = 0;

            /*! \brief Get user-defined Instance ID. 
            
                GetInstanceID() returns the arbitrary, user-defined instance ID of source
                instances. Texmaps can make use of this value at rendertime. This value
                can be negative or zero. */
            virtual __int64 GetInstanceID() = 0;

            /*! \brief Get material ID override.

            This function returns per-instance mesh matID overrides.
            A return value of -1 means no override is set on the instance.
            */
            virtual int GetMatID() = 0;

            /*! \brief Get material override.

            This function returns per-instance material (Mtl*) overrides.
            A return value of NULL means no override is set on the instance and
            thus the default node material should be used.
            */
            virtual Mtl* GetMtl() = 0;

            /*! \brief Get per-instance UVW channel overrides

            This function returns per-instance UVW overrides for specific map
            channels.
            The return value is an array which contains a list of overrides
            and the map channel whose vertices they should be assigned to. An
            empty array means no UVW overrides have been assigned to the instance.
            */
            virtual MaxSDK::Array<InstanceUVWInfo> GetUVWsVec() = 0;
            ///@}

            /*! \name Position and motion

            These functions return the position and movement of the instance.
            Exactly what is returned can be deduced from the MotionBlurInfo passed to
            RenderTimeInstancing::UpdateInstanceData()

            \note Any passed vertex velocity is in <em>addition</em> to this instance motion
            */
            ///@{
            /*! \brief Get the transformation matrix (or matrices)
            
            GetTMs() returns the instance's transform(s) spread evenly over the motion
            blur interval (the interval specified by the arguments passed to
            CollectInstances), in temporal order. 
            
            * If the vector returns has a single element it represents a static instance. 
            * If it has two elements it contains the transforms at the start and end of the interval. 
            * If it has three elements ut contains the transforms at the start, center, and end of
              the interval, etc. 

            A vector with more than two elements allows a renderer to compute more accurate multi-sample motion blur.
            */
            virtual MaxSDK::Array<Matrix3> GetTMs();

            /*! \brief Get instance veolocity.
            
            Returns the instance velocity of the instance in world space, measured in units per frame.
            */
            virtual Point3 GetVelocity() = 0;

            /*! \brief Get instance rotational velocity 
            
            Returns the spin is the per-frame instance spin of the instance, as an AngAxis in units per frame.
            */
            virtual AngAxis GetSpin() = 0;
            ///@}
        };

        /*! \brief Information about a given source, to be instanced multiple times */
        class RenderInstanceSource
        {
        public:
            /*! \brief Get the flags that define the type of data stored.

            Defines the type returned in the GetData() pointer, and any other relevant information,
            like whether the plugin must delete the pointer once it's finished using it.
            */
            virtual DataFlags GetFlags() = 0;

            /*! \brief Get the data pointer for that item that that should be instanced. 
            
            Currently, it is either a Mesh* or an INode*, and
            the flags returned by GetFlags() can be queried to find out which class type it is.

            The variable should only have one class type flag set, but may
            have other relevant information flagged as well, so one should not
            test for the class type with the equality ('==') operator, but instead
            bitwise AND operator ('&'). For example:
            \code
            if (flags == DataFlags::mesh)  {auto mesh = (Mesh*)data;} //incorrect
            if (flags == DataFlags::inode) {auto node = (INode*)data;} //incorrect

            if (flags & DataFlags::mesh)   {auto mesh = (Mesh*)data;} //correct
            if (flags & DataFlags::inode)  {auto node = (INode*)data;} //correct
            \endcode
            If the "pluginMustDelete" flag is set, the pointer should be deleted 
            after use. Be sure to cast to relevant class before deletion
            so the proper destructor is called.
            */
            virtual void *GetData() = 0;

            /*! \brief Get the velocity map channel, or -1 if none.

            This function returns the map channel where per-vertex
            velocity data (stored in units/frame) might be found, inside
            any meshes returned by the RenderTimeInstancing.

            A value of -1 means the mesh contains no per-vertex velocity data.

            \note Not all meshes are guaranteed to contain velocity data. It is
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
            the velocity map channel, through a RenderInstanceSource:
            \code
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
            \endcode
            */
            virtual int GetVelocityMapChannel() = 0;

            /*! \name Access to the instance targets
            */
            ///@{
            /*! \brief Get the number of instances of this source. */
            virtual size_t                GetNumInstanceTargets() = 0;
            /*! \brief Get the n:th instances of this source. */
            virtual RenderInstanceTarget *GetRenderInstanceTarget(size_t index) = 0;

            //! \brief For convenicence - iterator
            class Iterator;
            //! \brief Retreive the begin() iterator. Allows using a for (auto x : y) loop
            Iterator begin() { return Iterator(this); }
            //! \brief Retreive the end() iterator. 
            Iterator end() { return Iterator(this, GetNumInstanceTargets()); }
            class Iterator {
            public:
                Iterator(RenderInstanceSource *item) : m_item(item), m_i(0) {}
                Iterator(RenderInstanceSource *item, const size_t val) : m_item(item), m_i(val) {}

                Iterator&             operator++() { m_i++; return *this; }
                bool                  operator!=(const Iterator &iterator) { return m_i != iterator.m_i; }
                RenderInstanceTarget *operator*() { return m_item->GetRenderInstanceTarget(m_i); }
            private:
                size_t                 m_i;
                RenderInstanceSource  *m_item;
            };
        };

        inline RenderTimeInstancing* GetRenderTimeInstancing(BaseObject* obj)
        {
            return (RenderTimeInstancing*)obj->GetInterface(RENDERTIME_INSTANCING_INTERFACE);
        }
    }
}