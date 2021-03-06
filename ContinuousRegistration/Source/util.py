import subprocess, sys, logging, os
from itertools import islice
from datetime import datetime
import SimpleITK as sitk
import numpy as np


def take(iterable, n):
    "Return first n items of the iterable as a list"
    return list(islice(iterable, n))


def sort_file_names(file_names):
    return sorted(file_names, key=lambda dictionary: dictionary['image_file_names'][0])


# Some data sets come without world coordinate information for the label images. This is a problem
# when measuring overlap. Here, we copy information from the image to the label. We write in mha
# format so we can inspect the header with a simple text editor
copy_information_from_images_to_labels_ext = '.mha'
def copy_information_from_images_to_labels(image_file_names, label_file_names,
                                           disp_field_file_names,
                                           output_directory, mhd_pixel_type):
        new_label_file_names = []
        for image_file_name, label_file_name, disp_field_file_name \
                in zip(image_file_names, label_file_names, disp_field_file_names):
            label_file_name_we, label_file_name_ext = os.path.splitext(label_file_name)
            dataset_output_directory = os.path.join(output_directory, 'tmp', 'labels_with_world_info',
                                                    os.path.dirname(disp_field_file_name))
            output_file_name = os.path.join(dataset_output_directory,
                                            os.path.basename(label_file_name_we)
                                            + '_label_with_world_info'
                                            + copy_information_from_images_to_labels_ext)

            if not os.path.isdir(dataset_output_directory):
                os.makedirs(dataset_output_directory)

            # File info is read from corresponding image file
            image = sitk.ReadImage(image_file_name)

            # Write raw file with this info
            label = sitk.ReadImage(label_file_name)
            label.CopyInformation(image)

            if not os.path.isfile(output_file_name):
                sitk.WriteImage(sitk.Cast(label, sitk.sitkUInt8), output_file_name)
                print('Created label with world information %s.' % output_file_name)

            new_label_file_names.append(output_file_name)

        return tuple(new_label_file_names)


mask_ext = '.nii.gz'
def create_mask_by_thresholding(label_file_names, disp_field_file_names,
                                output_directory, threshold, dilate, erode):
    mask_file_names = []
    for label_file_name, disp_field_file_name \
            in zip(label_file_names, disp_field_file_names):
        label_file_name_we, label_file_name_ext = os.path.splitext(label_file_name)
        dataset_output_directory = os.path.join(output_directory, 'tmp', 'masks',
                                                os.path.dirname(disp_field_file_name))
        output_file_name = os.path.join(dataset_output_directory, os.path.basename(
            label_file_name_we) + '_mask' + mask_ext)

        if not os.path.isdir(dataset_output_directory):
            os.makedirs(dataset_output_directory)

        if not os.path.isfile(output_file_name):
            label = sitk.ReadImage(label_file_name)
            mask = label > threshold
            padding = (erode,)*mask.GetDimension()
            padded_mask = sitk.ConstantPad(mask, padding, padding)
            dilated_mask = sitk.BinaryDilate(padded_mask, dilate, sitk.sitkAnnulus, 0, 1, False) # pixels
            filled_mask = sitk.BinaryErode(dilated_mask, erode, sitk.sitkAnnulus, 0, 1, False)
            cropped_filled_mask = sitk.Crop(filled_mask, padding, padding)
            sitk.WriteImage(cropped_filled_mask, output_file_name)
            print('Created mask %s.' % output_file_name)

        mask_file_names.append(output_file_name)

    return tuple(mask_file_names)


def create_mask_by_size(image_file_name, mask_file_name):
    mask_directory = os.path.dirname(mask_file_name)

    if not mask_file_name.endswith(mask_ext):
        mask_file_name = os.path.splitext(mask_file_name)[0] + mask_ext

    if mask_directory is not None:
        os.makedirs(mask_directory, exist_ok=True)

    if not os.path.exists(mask_file_name):
        image = sitk.ReadImage(image_file_name)

        siz = image.GetSize()
        siz = siz[len(siz)-1:] + siz[:len(siz)-1]  # left-shift size
        mask = sitk.GetImageFromArray(np.ones(siz))
        mask.CopyInformation(image)

        os.makedirs(os.path.dirname(mask_file_name), exist_ok=True)
        sitk.WriteImage(sitk.Cast(mask, sitk.sitkUInt8), mask_file_name)

    return mask_file_name


def create_identity_world_information(file_names, dataset_name, input_directory, output_directory):
    new_file_names = []
    for file_name in file_names:
        output_file_name = os.path.join(output_directory, 'tmp', 'identity_world_information', dataset_name, file_name)

        if not os.path.isfile(output_file_name):
            file_output_directory = os.path.dirname(output_file_name)
            if not os.path.isdir(file_output_directory):
                os.makedirs(file_output_directory)

            image = sitk.ReadImage(os.path.join(input_directory, file_name))
            direction = np.identity(image.GetDimension()).flatten()
            image.SetDirection(direction)
            image.SetSpacing((1,) * image.GetDimension())
            image.SetOrigin((0,) * image.GetDimension())
            sitk.WriteImage(image, output_file_name)
            print('Created image %s with identity world information.' % output_file_name)

        new_file_names.append(output_file_name)

    return tuple(new_file_names)


def create_disp_field_names(image_file_names, name):
    disp_field_file_names = []
    for name_0, name_1 in image_file_names:
        name_0 = os.path.basename(name_0)
        name_1 = os.path.basename(name_1)
        name_we_0, image_extension_we_0 = os.path.splitext(name_0)
        name_we_1, image_extension_we_1 = os.path.splitext(name_1)
        name_pair_1 = name_we_1 + "_to_" + name_we_0 + ".mha"
        name_pair_0 = name_we_0 + "_to_" + name_we_1 + ".mha"
        disp_field_file_names.append((os.path.join(name, name_pair_1),
                                         os.path.join(name, name_pair_0)))
    return disp_field_file_names


def merge_dicts(*dicts):
    return { key: value for dict in dicts for key, value in dict.items() }


def load_vtk(file_name):
    return np.loadtxt(file_name, skiprows=5)


def load_pts(file_name):
    return np.loadtxt(file_name)


def load_csv(path_file):
    """ loading points from a CSV file as ndarray of floats

    :param str path_file:
    :return ndarray:

    >>> content = " ,X,Y\\n1,226.4,173.5\\n2,278,182\\n3,256.7,171.2"
    >>> _= open('sample_points.csv', 'w').write(content)
    >>> load_csv('sample_points.csv')
    array([[ 226.4,  173.5],
           [ 278. ,  182. ],
           [ 256.7,  171.2]])
    >>> os.remove('sample_points.csv')
    """
    with open(path_file, 'r') as fp:
        lines = fp.readlines()
    points = [list(map(float, l.rstrip().split(',')[1:])) for l in lines[1:]]
    return np.array(points)


def load_point_set(file_name):
    if file_name.endswith(".vtk"):
        return load_vtk(file_name)
    elif file_name.endswith(".csv"):
        return load_csv(file_name)
    else:
        return load_pts(file_name)


def txt2vtk(point_set_file_name, disp_field_file_name):
    # Avoid name clash by appending time in microseconds. This also makes the outputted files sortable
    output_file_name = os.path.splitext(disp_field_file_name)[0] \
                   + '-' + "{:%Y-%m-%d-%H-%M-%S-%f}".format(datetime.now()) + '.vtk'

    try:
        point_set = np.loadtxt(point_set_file_name)
    except Exception as e:
        raise Exception('Failed to load %s: %s' % (point_set_file_name, str(e)))

    try:
        with open(output_file_name, 'w+') as f:
            f.write("# vtk DataFile Version 2.0\n")
            f.write("Point set warp generated by SuperBench\n")
            f.write("ASCII\n")
            f.write("DATASET POLYDATA\n")
            f.write("POINTS %i float\n" % point_set.shape[0])

            for point in point_set:
                for p in point:
                    f.write("%f " % p)

                f.write("\n")
    except Exception as e:
        raise Exception('Error during txt2vtk: %s' % str(e))

    return output_file_name


def get_script_path():
    return os.path.dirname(os.path.realpath(sys.argv[0]))


def warp_point_set(superelastix, point_set_file_name, disp_field_file_name):
    blueprint_file_name = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'warp_point_set.json')

    output_point_set_file_name = os.path.splitext(disp_field_file_name)[0] + '-' \
                             + "{:%Y-%m-%d-%H-%M-%S-%f}".format(datetime.now()) + '.vtk'

    # Convert txt file to vtk format
    if not point_set_file_name.endswith(".vtk"):
        point_set_file_name = txt2vtk(point_set_file_name, disp_field_file_name)

    try:
        stdout = subprocess.check_output([
            superelastix,
            '--conf', os.path.join(get_script_path(), 'warp_point_set.json'),
            '--in', 'InputPointSet=%s' % point_set_file_name,
            'DisplacementField=%s' % disp_field_file_name,
            '--out', 'OutputPointSet=%s' % output_point_set_file_name,
            '--loglevel', 'trace',
            '--logfile', os.path.splitext(output_point_set_file_name)[0] + '.log'])
    except:
        # FIXME: no such file is created
        raise Exception('\nFailed to warp %s. See %s' %
                        (point_set_file_name, os.path.splitext(output_point_set_file_name)[0] + '.log'))

    return output_point_set_file_name


def warp_label_image(superelastix, label_file_name, disp_field_file_name):
    output_label_file_name = os.path.splitext(disp_field_file_name)[0] + '-' \
                             + "{:%Y_%m_%d_%H_%M_%S_%f}".format(datetime.now()) + '.nii'

    try:
        displacement_field = sitk.ReadImage(disp_field_file_name, sitk.sitkVectorFloat64)
        disp_field_transform = sitk.DisplacementFieldTransform(displacement_field)

        label_image = sitk.ReadImage(label_file_name)
        output_label_image = sitk.Resample(label_image,
                                     disp_field_transform,
                                     sitk.sitkNearestNeighbor,
                                     0,
                                     label_image.GetPixelID())

        sitk.WriteImage(output_label_image, output_label_file_name)
    except:
        logging.error('Failed to warp %s.' % label_file_name)


    return output_label_file_name
