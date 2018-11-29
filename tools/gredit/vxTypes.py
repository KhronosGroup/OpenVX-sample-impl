
class vxTypes:
    """ This is a container for the list of all supported types in OpenVX 1.0
    """
    data_objects = [ "vx_array", "vx_convolution", "vx_delay", "vx_distribution", "vx_image", "vx_lut", "vx_matrix", "vx_pyramid", "vx_remap", "vx_scalar", "vx_threshold"]
    frmk_objects = [ "vx_context", "vx_graph", "vx_node", "vx_kernel", "vx_parameter", "vx_target" ]

    @classmethod
    def objects(self):
        return self.data_objects + self.frmk_objects


