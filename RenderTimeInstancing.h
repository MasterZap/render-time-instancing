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
        //! \brief A Channel ID. An opaque integer token representing the channel. Used to actully retreive the data.
        typedef int ChannelID;
        enum        TypeID;

        class  RenderInstanceSource;
        struct MotionBlurInfo;
        struct ChannelInfo;

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
            with the functions GetChannels(). Each channel has a name, a type, and a ChannelID, which is used to
            retreive the actual values for each instance using the GetXXXX() functions.

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

                instancer->UpdateInstanceData(t, valid, mblur, view, _T("myPlugin"));

                // Get integers for known channel names

                ChannelID floatChannel1  = instancer->GetChannelID(_T("myFloatChannel"),   typeFloat);
                ChannelID VectorChannel1 = instancer->GetChannelID(_T("myVectorChannel1"), typeVector);
                ChannelID VectorChannel2 = instancer->GetChannelID(_T("myVectorChannel2"), typeVector);
                ChannelID TMChannel1     = instancer->GetChannelID(_T("myTMChannel"),      typeTM);

                // Instancer acts as a container of sources. Loop over the sources in the instancer
                for (auto source : *instancer)
                    auto flags = source->GetFlags();

                    // Get what to be instanced
                    void *data = source->GetData();

                    if (flags & DataFlags::mesh)  { ... the pointer is a mesh .... }
                    if (flags & DataFlags::inode) { ... the data pointer is an iNode .... }

                    // A source acts as a container of targets
                    for (auto target : *source )
                    {
                        // Deal with known data. Will return defaults if missing.
                        // It is the responsibility of the caller to not call the wrong type
                        float   f1  = target->GetCustomFloat(floatChannel1);
                        Point3  v1  = target->GetCustomVector(vectorChannel1);
                        Point3  v2  = target->GetCustomVector(vectorChannel2);
                        Matrix3 tm1 = target->GetCustomVector(TMChannel1);

                        // ... actually instance the "source" object using info in "target"

                        // If the renderer preffers to work with velocities and spins
                        // it should check if the object is providing such data.
                        // This case is not necessary if the renderer only cares about
                        // the array of transforms.
                        if (mblur.flags & MBFlags::mb_velocityspin)
                        {
                            Matrix3 tm = target->GetTM();
                            Vector3    = target->GetVelocity();
                            AngAxis    = target->GetSpin();

                            ... instance the obeject accordingly ...
                        }
                        else // There is no velocity/spin data, we use transforms
                        {
                            // Gets the array of transforms over the shutter interval
                            auto tms   = target->GetTms();

                            ... instance the obeject accordingly ...
                        }
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
                An initialized MotionBlurInfo, which upon return will contain info on how the 
                motion data is returned. If the object will return values for velocity and spin,
                it should set the mb_velocityspin flag. 

            @param view
                The view. This allows the object to do level-of-detail computation or do
                camera frustum culling.

            @param plugin
                The plugin argument of this function takes the name of the plugin
                querying this interface, in lowercase letters.
                Ex: _T("arnold"), _T("octane"), _T("redshift"), _T("vray"), etc.                
                This is a somewhat arbitrary value, but by having renderers identify
                themselves during a query, the object can internally determine if any
                renderer-specific edge-cases need to be processed.

            \see Struct MotionBlurInfo
            */
            virtual void UpdateInstanceData(TimeValue t, Interval &valid, MotionBlurInfo &mbinfo, View &view, TSTR plugin) = 0;

            /*! \brief Release the instancing data.

            When a caller of this interface is done with the data, it can call ReleseInstanceData().
            In case the generating plugin allocated some information, it can be released here.
            */
            virtual void ReleaseInstanceData() = 0;
            ///@}

            /*! \name Getting Data Channels 
                These are functions to obtain the  list of datachannels on the object. Known channels
                can also be requested by name. */
            ///@{
            //! \brief Return a list of data channels
            virtual MaxSDK::Array<ChannelInfo> GetChannels() = 0;
            //! \brief Utility function to get the channel ID of a known channel. 
            //         Returns -1 if a channel of that name and type does not exist.
            virtual ChannelID GetChannelID(TSTR name, TypeID type) = 0;
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
            ///@}
        };

        /*! \brief Motion Blur information struct.

        This communicates information about shutter intervals and motion blur behavior between the object and the renderer.
        
        It is filled in and passed RenterTimeInstancing::UpdateInstanceData() by the renderer and the object may modify it
        to communicate back to it. 
        */
        struct MotionBlurInfo {
            /*! \brief Defines MotionBlurInfo::flags means */
            enum MBFlags : signed int
            {
                mb_none = 0,       //!< \brief No flags (default)
                mb_velocityspin = 1 << 0,  //!< \brief The RenderInstanceTarget::GetVelocity() and RenderInstanceTarget::GetSpin() will contain data
            };
            /*! \brief Defines information about what motion blur info is available for a source
            \see Enum MBFlags */
            MBFlags   flags;            
            /*! \brief Defines the open and closing time of the shutter.
            If set to NEVER, motion blur is not used. */
            Interval  shutterInterval;  

            MotionBlurInfo(Interval shutter = NEVER, MBFlags f = MBFlags::mb_none) { 
                shutterInterval = shutter;
                flags           = f;
            };
        };

        /*! \brief UVW channel override data */
        struct InstanceUVWInfo { int channel; UVVert value; };

        struct ChannelInfo {
            TSTR name;            //!< \brief The name of the data channel
            //! \brief Type of the channel
            enum TypeID : int {
                typeCustom = 0,   //!< \brief Custom data block of specified size
                typeInt    = 1,   //!< \brief Data of type int
                typeFloat  = 2,   //!< \brief Data of type float
                typeVector = 3,   //!< \brief Data of type Point3
                typeColor  = 4,   //!< \brief Data of type Color. Colors and vectors may hold semantic difference to some renderers.
                typeTM     = 5    //!< \brief Data of type Matrix3
            };
            TypeID    type;       //!< \brief The type of channel
            ChannelID channelID;  //!< \brief The channels ID. An opaque integer token representing the channel. Used to actully retreive the data.
            int size;             //!< \brief For typeCustom only - the size of the data, in case the renderer needs to make a copy of it.
        };

        /*! \brief Defines what GetData() returns and how to treat it */
        enum DataFlags : signed int
        {
            df_none  = 0,

            df_mesh  = 1 << 0, //!< \brief RenderInstanceSource::GetData() is Mesh*
            df_inode = 1 << 1, //!< \brief RenderInstanceSource::GetData() is INode*

            df_pluginMustDelete = 1 << 31 //!< \brief Set if the renderer is expected to delete the data pointer after use
        };

        /*! \brief Information about a given instance of a RenderInstanceSource */
        class RenderInstanceTarget
        {
        public:

            /*! \name Instance Custom Data Access
            
            These functions return custom data values for each
            instance. Values are retreived using ChannelID's 
            */
            ///@{
            //! \brief Return a raw custom data pointer. If the ID is invalid, returns nullptr.
            virtual void   *GetCustomData  (ChannelID channel);  
            //! \brief Return a float value
            virtual float   GetCustomFloat (ChannelID channel)   
            {
                auto p = GetCustomData(channel);
                return p ? (*((float *)p)) : 0.0f;
            }
            //! \brief Return a vector value
            virtual Point3  GetCustomVector(ChannelID channel)   
            {
                auto p = GetCustomData(channel);
                return p ? (*((Point3 *)p)) : Point3(0.0f,0.0f,0.0f);
            }
            //! \brief Return a color value
            virtual Color   GetCustomColor(ChannelID channel)   
            {
                auto p = GetCustomData(channel);
                return p ? (*((Color *)p)) : Color(0.0f,0.0f,0.0f);
            }
            //! \brief Return a TM value
            virtual Matrix3 GetCustomTM    (ChannelID channel)   
            {
                auto p = GetCustomData(channel);
                return p ? (*((Matrix3 *)p)) : Matrix3();
            }
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

            Any instance motion should be computed from <em>either</em> multiple returned
            tranformations from GetTMs() <em>or</em> by using only one transform from
            GetTM() and applying the GetVelocity() and GetSpin() values on top of that. 
            Both methods should never be used at the same time. The method using velocity
            and spin should only be used if the object signals that it has this information
            using the MBData::mb_velocityspin flag. I.e. transformation matrices are
            always there but may or may not be computed <em>from</em> velocity and spin
            data. The velocity and spin is only guaranteed to be there if the flag in 
            question is set.

            \note Any passed vertex velocity is in <em>addition</em> to this instance motion
            */
            ///@{
            /*! \brief Get the transformation matrix (or matrices)
            
            GetTMs() returns the instance's transform(s) spread evenly over the motion
            blur interval (the interval specified by the MotionBlurInfo argument passed to
            UpdateInstanceData function), in temporal order. 
            
            - If the vector returns has a single element it represents a static instance that is not moving.
            - If it has two elements it contains the transforms at the start and end of the interval. 
            - If it has three elements ut contains the transforms at the start, center, and end of
              the interval, etc. 

            A vector with more than two elements allows a renderer to compute more accurate multi-sample motion blur.
            */
            virtual MaxSDK::Array<Matrix3> GetTMs() = 0;

            /*! \brief Get the transformation matrix at shutter open only

            GetTM() returns the instance's transform. This can be more efficient if the object is actually
            internally using a single transform and a velocity and spin, saving it the effort of computing
            the additional matrices. If a renderer does not plan to use multiple matrices, and compute motion
            blur using GetVelocity() and GetSpin(), it can call this function instead of using GetTMs()[0],
            which while equivalent, might be slightly less efficient.
            */
            virtual Matrix3 GetTM() = 0;

            /*! \brief Get instance veolocity.
            
            Returns the instance velocity of the instance in world space, measured in units per tick.
            */
            virtual Point3 GetVelocity() = 0;

            /*! \brief Get instance rotational velocity 
            
            Returns the spin is the per-frame instance spin of the instance, as an AngAxis in units per tick.
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

            int velMapChan = renderInstanceSource->GetMeshVelocityMapChannel();
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
            Iterator end()   { return Iterator(this, GetNumInstanceTargets()); }
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